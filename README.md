# QTWebBrowser + VNC_Client

A dual application system built with **Qt 6.8.2**, designed to demonstrate a cloud based browsing experience. The primary component, `QTWebBrowser`, embeds a custom built **VNC Server** which allows remote control via a secondary application, `VNC_Client`. The client connects using the **RFB 3.8 protocol**, implemented entirely from scratch.

This project is built using **CMake** and **MSVC** (Microsoft Visual C++) for compilation on Windows.

---

## Features

- **QTWebBrowser**:
  - Web browser interface powered by Qt WebEngine
  - Integrated VNC Server for remote control
  - Real time interaction and rendering over the network
  - Written with custom RFB 3.8 protocol support

- **VNC_Client**:
  - Lightweight Qt based VNC viewer
  - Control and view the browser remotely
  - Written with custom RFB 3.8 protocol support

- **Custom Protocol Implementation**:
  - No external VNC libraries used
  - Full control over the protocol stack for learning and customization

- **Known Issues:**
    - **Incorrect Encoding Type on Connection:**
      When the VNC client connects, the initial encoding type sent or received is incorrect, leading to rendering issues. This is a known bug and will be addressed in future updates.

---

## Build Instructions

### Prerequisites

- Qt 6.8.2
- CMake (>= 3.20)
- QT Creator 16.0.0 Comunity or Visual Studio with MSVC toolset 
- Git (optional, for cloning)

### Steps

1. **Clone the repository**:
   ```bash
   git clone https://github.com/aidan729/vnc-web-browser.git
   cd qt-vnc-browser

---

###Project Status

This project started as a learning demonstration for understanding Qt in preparation for a job opportunity. However, I decied that this could be a fun project to continue working on, so now the repo is now open source and welcoming contributions!

Whether you're fixing bugs, improving performance, or suggesting new features pull requests are encouraged!

### Contributing
  - Fork the repository
  - Create a new branch (```git checkout -b feature-xyz```)
  - Commit your changes (```git commit -am 'Add feature'```)
  - Push to the branch (```git push origin feature-xyz```)
  - Open a Pull Request

