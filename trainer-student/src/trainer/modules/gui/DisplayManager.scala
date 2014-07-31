package trainer.modules.gui

import java.awt.Toolkit

import scala.collection.mutable.ArrayBuffer

import libgurra.math.Vec2i
import trainer.AppConfig
import trainer.modules.decompressor.ShmStream

object WP {
    def apply(txt: String): Seq[Int] = {
        txt.split(",").map(_.toInt)
    }
}

case class DisplaySpace(val x: Int, val y: Int, val w: Int, val h: Int) {
    private def this(ints: Seq[Int]) = this(ints(0), ints(1), ints(2), ints(3))
    def this(txt: String) = this(WP(txt))
    override def toString(): String = {
        s"$x,$y,$w,$h"
    }
}

class DisplayManager {

    private val screenSize = Toolkit.getDefaultToolkit().getScreenSize();
    private val screenWidth = screenSize.getWidth();
    private val screenHeight = screenSize.getHeight();

    private val windows = new ArrayBuffer[InboundStreamWindow]
    private val initSize = new Vec2i(400, 400)

    private def findNewDisplaySpace(window: InboundStreamWindow): DisplaySpace = {
        new DisplaySpace(100, 100, 400, 400)
    }

    private def getCurrentDisplaySpace(window: InboundStreamWindow): DisplaySpace = {
        new DisplaySpace(
            window.getLocationOnScreen().x,
            window.getLocationOnScreen().y,
            window.getWidth(),
            window.getHeight())
    }

    private def saveLocation(window: InboundStreamWindow) {
        AppConfig.setProperty(window.toString(), getCurrentDisplaySpace(window).toString())
    }

    def find(source: ShmStream): Option[InboundStreamWindow] = {
        windows.find(_.source == source)
    }

    def existWindowForSource(source: ShmStream): Boolean = {
        find(source).isDefined
    }

    def add(source: ShmStream, mkWindow: DisplayManager => InboundStreamWindow) {

        if (!existWindowForSource(source)) {

            val window = mkWindow(this)

            val space = new DisplaySpace(AppConfig.getOrUpdate(window.toString(), findNewDisplaySpace(window).toString()))

            window.setLocation(space.x, space.y)
            window.setSize(space.w, space.h)

            windows += window

        }
    }

    def remove(window: InboundStreamWindow) {
        if (window.isVisible())
            saveLocation(window)
        windows -= window
        window.dispose()
    }

    def remove(stream: ShmStream) {
        find(stream).foreach(remove(_))
    }

    def shutdown() {
        windows.reverse.foreach(remove(_))
    }

}