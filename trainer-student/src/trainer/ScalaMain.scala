package trainer

import javax.swing.ToolTipManager
import libgurra.swing.SwingWrapped
import trainer.modules.gui.Gui
import trainer.modules.server.Server

object ScalaMain {

    def main(args: Array[String]): Unit = {

        AppConfig.loadFromDefaultFile()
        ToolTipManager.sharedInstance().setInitialDelay(0)
        ToolTipManager.sharedInstance().setReshowDelay(0)

        println(s"Started $this")
        val gui = new SwingWrapped(new Gui())
        val server = new Server(gui)
        server.start()
        gui.waitUntilClosed()

        server.kill()
        server.join()
        println(s"Finished $this")

        AppConfig.saveToDefaultFile()

    }

}