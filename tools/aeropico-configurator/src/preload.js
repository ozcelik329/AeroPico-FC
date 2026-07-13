const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("aeropicoBridge", {
  onPortList: (callback) => {
    ipcRenderer.on("serial-port-list", (_event, list) => callback(list));
  },
  chooseSerialPort: (portId) => {
    ipcRenderer.send("serial-port-choice", typeof portId === "string" ? portId : "");
  }
});
