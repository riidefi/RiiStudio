/**
 * @file
 * @brief Headers for the root platform/window interface. Implemented in
 * impl/glfw_win32.cpp, impl/sdl_ems.cpp
 */

#pragma once

#include <memory>
#include <queue>
#include <span>
#include <stdint.h>
#include <string>
#include <string_view>

namespace plate {

struct FileData {
  std::unique_ptr<uint8_t[]> mData;
  std::size_t mLen;
  std::string mPath;

  FileData(std::unique_ptr<uint8_t[]> data, std::size_t len,
           const std::string& path)
      : mData(std::move(data)), mLen(len), mPath(path) {}
};

class Platform {
public:
  /**
   * @brief        The constructor.
   *
   * @param width  Initial width of the window.
   * @param height Initial height of the window.
   * @param title  Initial title of the window. This will be the name of the
   *			   browser tab on web platforms.
   */
  Platform(unsigned width, unsigned height, const std::string& title);

  /**
   * @brief The destructor.
   */
  virtual ~Platform();

  /**
   * @brief Enter the main loop.
   */
  void enter();

protected:
  /**
   * @brief Process each frame.
   */
  virtual void rootCalc() {}

  /**
   * @brief Render each frame.
   */
  virtual void rootDraw() {}

public:
  /**
   * @brief      Handle a file being dropped on the application.
   *
   * @details    Called at the beginning of each frame, before calc/draw.
   *
   * @param data File data.
   * @param len  Length of the data buffer.
   * @param name Path of the file on disc.
   */
  virtual void vdropDirect(std::unique_ptr<uint8_t[]> data, std::size_t len,
                           const std::string& name) {}

  /**
   * @brief Write a file to disc. On the web backend, the file will
   * instead be supplied as a download.
   *
   * @param data Binary blob.
   * @param path Path to write the file at.
   * 
   */
  static void writeFile(const std::span<uint8_t> data, const std::string_view path);

  void hideMouse();
  void showMouse();

  void setVsync(bool enabled);

  void* getPlatformWindow() { return mPlatformWindow; }
  std::string getTitle() const { return mTitle; }

  void enqueueDrop(const std::string& path) { mDropQueue.push(path); }

private:
  void* mPlatformWindow = nullptr;
  std::string mTitle;

  std::queue<std::string> mDropQueue;
  std::queue<FileData> mDataDropQueue;

#ifdef RII_BACKEND_SDL
public:
  void mainLoopInternal();
#endif
};

} // namespace plate
