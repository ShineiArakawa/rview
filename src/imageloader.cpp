#include <imageloader.h>

// ###########################################################################################################################################
// ThreadPool
// ###########################################################################################################################################

ThreadPool::ThreadPool(int numThreads)
    : _workers(),
      _taskMutex(),
      _isRunning(true),
      _condition(),
      _tasks() {
  // Create and launch a number of worker threads
  for (int i = 0; i < numThreads; ++i) {
    _workers.emplace_back(&ThreadPool::worker, this);
  }
}

ThreadPool::~ThreadPool() {
  {
    // Lock task queue to prevent adding new task.
    std::lock_guard<std::mutex> lock(_taskMutex);
    _isRunning = false;
  }

  _condition.notify_all();  // Notify all threads to wake up and exit

  for (auto& worker : _workers) {
    if (worker.joinable()) {
      worker.join();  // Wait for all threads to finish
    }
  }

  _workers.clear();  // Clear the vector of threads
}

template <typename F>
auto ThreadPool::submit(F&& func) -> std::future<std::invoke_result_t<F>> {
  using R = std::invoke_result_t<F>;

  auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(func));
  auto future = task->get_future();

  pushTask([task]() {
    (*task)();
  });

  return future;
}

template <typename F>
void ThreadPool::pushTask(const F& task) {
  {
    std::lock_guard<std::mutex> lock(_taskMutex);

    if (_isRunning) {
      _tasks.push(std::function<void()>(task));  // Add the task to the queue
    } else {
      throw std::runtime_error("ThreadPool is not running anymore.");
    }
  }

  _condition.notify_one();  // Notify one thread to wake up and execute the task
}

void ThreadPool::worker() {
  for (;;) {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> lock(_taskMutex);
      _condition.wait(lock, [&] { return !_isRunning || !_tasks.empty(); });

      if (!_isRunning && _tasks.empty()) {
        return;  // Exit if the pool is not running and there are no tasks
      }

      task = std::move(_tasks.front());  // Get the next task
      _tasks.pop();                      // Remove the task from the queue
    }

    task();  // Execute the task
  }
}

// ###########################################################################################################################################
// AsyncImageLoader
// ###########################################################################################################################################

AsyncImageLoader::AsyncImageLoader(int numThreads, int numPreloadedImages)
    : _threadPool(std::make_shared<ThreadPool>(numThreads)),
      _images(),
      _imageMutex(),
      _futures(),
      _numPreloadedImages(numPreloadedImages),
      _imagePaths() {
}

AsyncImageLoader::~AsyncImageLoader() {
  _images.clear();  // Clear the map of images
}

void AsyncImageLoader::loadImageImpl(const fs::path& filePath, std::promise<ImageData>&& promise) {
  try {
    if (!fs::exists(filePath)) {
      throw std::runtime_error("File does not exist.");
    }
    if (!fs::is_regular_file(filePath)) {
      throw std::runtime_error("File is not a regular file.");
    }

    cv::Mat image = cv::imread(FileUtil::pathToString(filePath), cv::IMREAD_UNCHANGED);
    if (image.empty()) {
      throw std::runtime_error("Failed to load image.");
    }

    // Convert the image to RGBA format
    cv::Mat rgbaImage;
    if (image.channels() == 1) {
      cv::cvtColor(image, rgbaImage, cv::COLOR_GRAY2RGBA);
    } else if (image.channels() == 3) {
      cv::cvtColor(image, rgbaImage, cv::COLOR_BGR2RGBA);
    } else if (image.channels() == 4) {
      rgbaImage = image;
    } else {
      qDebug() << "Unsupported image format";
      return;
    }

    // Convert the image to float32 format range [0, 1]
    double minVal, maxVal;
    cv::minMaxLoc(rgbaImage, &minVal, &maxVal);
    rgbaImage.convertTo(rgbaImage, CV_32F, 1.0 / (maxVal - minVal), -minVal / (maxVal - minVal));

    // Flip the image vertically
    cv::flip(rgbaImage, rgbaImage, 0);

    ImageData imageData(rgbaImage.clone(), filePath);  // Clone the image to avoid dangling reference

    {
      std::lock_guard<std::mutex> lock(_imageMutex);  // Lock the mutex to protect shared data
      _images[filePath] = imageData;                  // Store the image data in the map
    }
  } catch (const std::exception& e) {
    promise.set_exception(std::make_exception_ptr(std::runtime_error(e.what())));  // Set the exception in the promise
  }
}

void AsyncImageLoader::loadImages(const std::vector<fs::path>& filePaths) {
  _imagePaths = filePaths;  // Store the paths of the images to be loaded

  _futures.clear();  // Clear the futures map
  _images.clear();   // Clear the images map

  // 最初に読み込む画像の数を決定
  const size_t numImagesToLoad = std::min(static_cast<size_t>(_numPreloadedImages), filePaths.size());
  std::vector<fs::path> imagesToLoad(numImagesToLoad);
  std::copy_n(filePaths.begin(), numImagesToLoad, imagesToLoad.begin());  // Copy the paths to load

  for (const auto& filePath : imagesToLoad) {
    std::promise<ImageData> promise;
    auto future = promise.get_future();

    _futures[filePath] = std::move(future);  // Store the future in the map

    _threadPool->submit([this, filePath, promise = std::move(promise)]() mutable {
      loadImageImpl(filePath, std::move(promise));
    });
  }
}

ImageData AsyncImageLoader::getImage(const fs::path& filePath) {
  std::lock_guard<std::mutex> lock(_imageMutex);  // Lock the mutex to protect shared data

  auto it = _images.find(filePath);
  if (it != _images.end()) {
    qDebug() << "Image found in cache:" << filePath.string().c_str();
    return it->second;  // Return the image data if found
  }

  // If not found, wait for the future to be ready and get the result
  auto futureIt = _futures.find(filePath);
  if (futureIt != _futures.end()) {
    qDebug() << "Waiting for image to load:" << filePath.string().c_str();
    return futureIt->second.get();  // Get the image data from the future
  }

  return ImageData();  // Return an empty ImageData if not found
}
