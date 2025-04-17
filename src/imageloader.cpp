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
      _imageMutex(),
      _futures(),
      _numPreloadedImages(numPreloadedImages),
      _imagePaths() {
}

AsyncImageLoader::~AsyncImageLoader() = default;

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
      cv::cvtColor(image, rgbaImage, cv::COLOR_BGRA2RGBA);
    } else {
      qDebug() << "Unsupported image format";
      return;
    }

    // Convert the image to float32 format range [0, 1]
    double minVal, maxVal;
    cv::minMaxLoc(rgbaImage, &minVal, &maxVal);
    rgbaImage.convertTo(rgbaImage, CV_32F, 1.0 / (maxVal - minVal), -minVal / (maxVal - minVal));

    // Correct the orientation using EXIF data
    rgbaImage = ImagingUtil::correctOrientation(rgbaImage, filePath);

    // Flip the image vertically
    cv::flip(rgbaImage, rgbaImage, 0);

    ImageData imageData(rgbaImage.clone(), filePath);  // Clone the image to avoid dangling reference

    {
      std::lock_guard<std::mutex> lock(_imageMutex);  // Lock the mutex to protect shared data
      _imageCache[filePath] = imageData;              // Cache the loaded image
      promise.set_value(imageData);                   // Set the value in the promise
    }

#if defined(RVIEW_DEBUG_BUILD)
    qDebug() << "Image loaded:" << FileUtil::pathToQString(filePath);
#endif
  } catch (...) {
    std::exception_ptr ep = std::current_exception();
    promise.set_exception(ep);
  }
}

void AsyncImageLoader::loadImages(const std::vector<fs::path>& filePaths) {
  std::lock_guard<std::mutex> lock(_imageMutex);  // Lock the mutex to protect shared data

  _imagePaths = filePaths;  // Store the paths of the images to be loaded

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
  ImageData imageData;

  // ------------------------------------------------------------------------------------------------------------
  // Check if the file is included in file entries
  // ------------------------------------------------------------------------------------------------------------
  if (const auto& it = std::find(_imagePaths.begin(), _imagePaths.end(), filePath); it == _imagePaths.end()) {
    // Not found in the list of paths to load
    qInfo() << "File is not included in file entries: " << FileUtil::pathToQString(filePath);
    return imageData;
  }

  // ------------------------------------------------------------------------------------------------------------
  // Load from cache
  // ------------------------------------------------------------------------------------------------------------
  {
    std::lock_guard<std::mutex> lock(_imageMutex);

    if (_imageCache.find(filePath) != _imageCache.end()) {
      imageData = _imageCache.at(filePath);
    }
  }

  // ------------------------------------------------------------------------------------------------------------
  // Load from future
  // ------------------------------------------------------------------------------------------------------------
  if (imageData.empty()) {
    std::shared_future<ImageData> future;

    {
      std::lock_guard<std::mutex> lock(_imageMutex);

      auto futureIt = _futures.find(filePath);
      if (futureIt != _futures.end()) {
        future = futureIt->second.share();
      }
    }

    if (future.valid()) {
      // If the future is valid, wait for it to be ready and get the result
      imageData = future.get();  // Wait for the image to be loaded (blocking call)

      {
        // Erase the future from the map
        // NOTE: Once you call get() on a future, it will no longer be valid.
        std::lock_guard<std::mutex> lock(_imageMutex);

        auto futureIt = _futures.find(filePath);
        if (futureIt != _futures.end()) {
          _futures.erase(futureIt);  // Remove the future from the map
        }
      }
    }
  }

  // ------------------------------------------------------------------------------------------------------------
  // Add to the queue
  // ------------------------------------------------------------------------------------------------------------
  {
    // Add the image path to the list of paths to load
    std::vector<fs::path> pathsToLoadAll;
    pathsToLoadAll.reserve(_numPreloadedImages);

    const int currentIndex = std::distance(_imagePaths.begin(), std::find(_imagePaths.begin(), _imagePaths.end(), filePath));
    const int startIndex = std::max(currentIndex - _numPreloadedImages / 2 + 1, 0);

    for (int i = 0; i < _numPreloadedImages; ++i) {
      const int index = startIndex + i;

      if (index < static_cast<int>(_imagePaths.size())) {
        pathsToLoadAll.push_back(_imagePaths[index]);
      }
    }

    {
      std::lock_guard<std::mutex> lock(_imageMutex);

      std::vector<fs::path> pathsToLoad(pathsToLoadAll.size());
      // Preferencially load the image current filePath
      pathsToLoad.push_back(filePath);
      for (const auto& path : pathsToLoadAll) {
        if (path != filePath) {
          pathsToLoad.push_back(path);
        }
      }

      // Erase the futures that are not in the paths to load
      for (auto it = _futures.begin(); it != _futures.end();) {
        if (std::find(pathsToLoadAll.begin(), pathsToLoadAll.end(), it->first) == pathsToLoadAll.end()) {
          it = _futures.erase(it);
        } else {
          ++it;
        }
      }

      // Erase the paths that are already in the future
      for (auto it = pathsToLoad.begin(); it != pathsToLoad.end();) {
        if (_futures.find(*it) != _futures.end()) {
          it = pathsToLoad.erase(it);
        } else {
          ++it;
        }
      }

      // Add the new image paths to the queue
      for (const auto& path : pathsToLoad) {
        std::promise<ImageData> promise;
        auto future = promise.get_future();

        _futures[path] = std::move(future);

        _threadPool->submit([this, path, promise = std::move(promise)]() mutable {
          loadImageImpl(path, std::move(promise));
        });
      }

      // Erase cache images that are not in the paths to load
      for (auto it = _imageCache.begin(); it != _imageCache.end();) {
        if (std::find(pathsToLoadAll.begin(), pathsToLoadAll.end(), it->first) == pathsToLoadAll.end()) {
          it = _imageCache.erase(it);
        } else {
          ++it;
        }
      }
    }
  }

  // ------------------------------------------------------------------------------------------------------------
  // Try again to load from future
  // ------------------------------------------------------------------------------------------------------------
  if (imageData.empty()) {
    std::shared_future<ImageData> future;

    {
      std::lock_guard<std::mutex> lock(_imageMutex);

      auto futureIt = _futures.find(filePath);
      if (futureIt != _futures.end()) {
        future = futureIt->second.share();
      }
    }

    if (future.valid()) {
      // Wait for the image to be loaded (blocking call)
      imageData = future.get();

      {
        // Erase the future from the map
        std::lock_guard<std::mutex> lock(_imageMutex);

        // Remove the future from the map
        auto futureIt = _futures.find(filePath);
        if (futureIt != _futures.end()) {
          _futures.erase(futureIt);
        }
      }
    }
  }

#if defined(RVIEW_DEBUG_BUILD)
  // ------------------------------------------------------------------------------------------------------------
  // Check
  // ------------------------------------------------------------------------------------------------------------
  {
    std::lock_guard<std::mutex> lock(_imageMutex);

    if (_futures.size() > _numPreloadedImages) {
      qDebug() << "The number of futures " << _futures.size() << " exceeds the number of preloaded images " << _numPreloadedImages;
    }
    if (_imageCache.size() > _numPreloadedImages) {
      qDebug() << "The number of cached images " << _imageCache.size() << " exceeds the number of preloaded images " << _numPreloadedImages;
    }
  }
#endif

  return imageData;  // Return an empty ImageData if not found
}
