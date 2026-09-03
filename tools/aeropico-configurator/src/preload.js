const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("aeropicoBridge", {
  onPortList: (callback) => {
    ipcRenderer.on("serial-port-list", (_event, list) => callback(list));
  },
  onPortAutoSelected: (callback) => {
    ipcRenderer.on("serial-port-auto-selected", (_event, port) => callback(port));
  },
  onPortSelectionError: (callback) => {
    ipcRenderer.on("serial-port-selection-error", (_event, message) => callback(message));
  },
  chooseSerialPort: (portId) => {
    ipcRenderer.send("serial-port-choice", typeof portId === "string" ? portId : "");
  },
  cancelSerialPort: () => {
    ipcRenderer.send("serial-port-cancel");
  }
});
