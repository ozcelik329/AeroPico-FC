const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("aeropicoBridge", {
  onPortList: (callback) => {
    ipcRenderer.on("serial-port-list", (_event, list) => callback(list));
  },
  chooseSerialPort: (portId) => {
    ipcRenderer.send("serial-port-choice", typeof portId === "string" ? portId : "");
  },
  nativeListPorts: () => ipcRenderer.invoke("native-serial-list"),
  nativeConnect: (options) => ipcRenderer.invoke("native-serial-connect", options),
  nativeWrite: (bytes) => ipcRenderer.invoke("native-serial-write", Array.from(bytes || [])),
  nativeDisconnect: () => ipcRenderer.invoke("native-serial-disconnect"),
  onNativeData: (callback) => {
    ipcRenderer.on("native-serial-data", (_event, bytes) => callback(new Uint8Array(bytes)));
  },
  onNativeError: (callback) => {
    ipcRenderer.on("native-serial-error", (_event, message) => callback(message));
  }
});
