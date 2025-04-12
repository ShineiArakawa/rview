#pragma once

#include <fileutil.h>
#include <image.h>

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <queue>
#include <thread>
#include <vector>

// ###########################################################################################################################################
// ThreadPool
// ###########################################################################################################################################

class ThreadPool {
  // https://contentsviewer.work/Master/software/cpp/how-to-implement-a-thread-pool/article

 public:
  ThreadPool(int numThreads);
  ~ThreadPool();

  template <typename F>
  auto submit(F&& func) -> std::future<std::invoke_result_t<F>>;

 private:
  std::vector<std::thread> _workers;
  mutable std::mutex _taskMutex;
  bool _isRunning;
  std::condition_variable _condition;
  std::queue<std::function<void()>> _tasks;

  template <typename F>
  void pushTask(const F& task);
  void worker();
};

using ThreadPool_t = std::shared_ptr<ThreadPool>;

// ###########################################################################################################################################
// AsyncImageLoader
// ###########################################################################################################################################

class AsyncImageLoader {
 public:
  AsyncImageLoader(int numThreads,
                   int numPreloadedImages);
  ~AsyncImageLoader();

  void loadImageImpl(const fs::path& filePath, std::promise<ImageData>&& promise);

  void loadImages(const std::vector<fs::path>& filePaths);
  ImageData getImage(const fs::path& filePath);

 private:
  ThreadPool_t _threadPool;

  std::mutex _imageMutex;
  std::map<fs::path, std::future<ImageData>> _futures;

  int _numPreloadedImages;
  std::vector<fs::path> _imagePaths;

  std::map<fs::path, ImageData> _imageCache;
};

using AsyncImageLoader_t = std::shared_ptr<AsyncImageLoader>;
