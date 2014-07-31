package trainer.modules.server

import scala.collection.mutable.HashMap

import libgurra.net.TcpClient
import libgurra.net.TcpServer
import libgurra.swing.SwingWrapped
import trainer.modules.decompressor.ShmStream
import trainer.modules.gui.Gui
import trainer.modules.msgs.ShmNotifiationParser
import trainer.modules.msgs.ShmNotificationMsg

object Server {
    type Client = TcpClient[ShmNotificationMsg]
}

class Server(gui: SwingWrapped[Gui], port: Int = 12345) extends TcpServer[ShmNotificationMsg](port) {

    private val streams = new HashMap[Client, ShmStream]

    override def handleMessage(c: Client, msg: ShmNotificationMsg) {
        streams.getOrElseUpdate(c, new ShmStream(msg.shmName, gui)).updateWindow()
    }

    override def notifyConnected(c: Client) {
    }

    override def notifyDisconnected(c: Client) {
        streams.remove(c).foreach(_.kill())
    }

    /**
     * *******************************************
     *
     *
     * 			Private stuff
     *
     * *******************************************
     */

    override def mkClientParser() = new ShmNotifiationParser
    override def getCycleInterval() = 0.01
    override def getClientTimeout() = 2.0
    override def getClientSocketBufSizes() = 20 * 1024
    override def useTcpNoDelay() = true
    override def cycle(t: Double, dt: Double) {}
}