# 🟢 MatrixClassix ScreenSaver
**A high-performance Windows Screen Saver with a custom "Message Decoder" engine.**

---

## 👨‍💻 Developer Notes
I’m a Computer Science student who couldn't find a Matrix screensaver that felt "right" or offered modern customization.

I built this to practice **Win32 API programming** and state-machine logic.


### Technical Stack
* **Language:** C++
* **Graphics:** Win32 GDI/GDI+ (Native Windows rendering for low overhead)
* **Logic:** State-based animation (Scrolling -> Merging -> Decoding -> Separating)
* **Persistence:** Windows Registry (User settings storage)
* **Installer:** Inno Setup (Handles System32 deployment)

---

## ✨ Features
* **The Decoder:** After the initial 'rain', the system "glitches" to reveal your custom message.
* **Full Customization:** Change the message, speed, and colors via the native Windows Screen Saver settings.
* **Optimized:** Written in pure C++ with no heavy engines, ensuring it won't lag your PC.

---

## 🛠️ Installation
1. Download `MatrixClassix_Setup.exe`.
2. Run the installer. 
   * *Note: Since this is an indie project, click **"More Info"** -> **"Run Anyway"** if Windows SmartScreen appears.*
3. The installer will automatically open your **Screen Saver Settings**.
4. Select **MatrixClassix** and click **Settings** to set your secret message and preferences.

I hope you enjoy! :)