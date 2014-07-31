package trainer.modules.decompressor

import java.io.IOException

import libgurra.swing.SwingWrapped
import shm.SharedMemory
import trainer.modules.gui.Gui
import trainer.modules.gui.InboundStreamWindow
import trainer.modules.msgs.StreamPacket
import trainer.modules.msgs.StreamPacketHeader

class ShmStream(val shmName: String, val gui: SwingWrapped[Gui]) {

    val shm = new SharedMemory(shmName, 10 * 1024 * 1024, false)

    println("Opening shm " + shm.name)

    if (!shm.valid()) {
        closeShm()
        throw new IOException(s"Could not open shm $shmName")
    }

    val shmHeader = StreamPacket.parseHeader(shm.read(StreamPacketHeader.SIZE))

    gui.modify(_.mainGui.onInboundConnected(this))

    def updateWindow() { // The modify_Now_ here is required not to overflow with incoming draw instructions
        gui.modifyNow(_.displayManager.find(this).foreach {
            _.paintBytes({
                shm.read(StreamPacketHeader.SIZE, _, 0, shmHeader.width * shmHeader.height)
            }, shmHeader.width, shmHeader.height)
        })
    }

    def kill() {
        println("Closing shm " + shm.name)
        gui.modifyNow(_.mainGui.onInboundDisconnected(this))
        closeShm()
    }

    def callsign(): String = {
        shmHeader.callsign
    }

    def streamName(): String = {
        shmHeader.streamName
    }

    def openWindow() {
        gui.modify(_.displayManager.add(this, new InboundStreamWindow(this, _)))
    }

    def closeWindow() {
        gui.modify(_.displayManager.remove(this))
    }

    private def closeShm() {
        shm.close()
    }

}