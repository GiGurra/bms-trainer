package trainer.modules.gui

import java.awt.Component
import java.awt.Graphics
import java.awt.Graphics2D
import java.awt.RenderingHints
import java.awt.event.KeyAdapter
import java.awt.event.KeyEvent
import java.awt.image.BufferedImage
import java.awt.image.DataBufferInt
import javax.imageio.ImageIO
import javax.swing.JFrame
import javax.swing.WindowConstants
import trainer.modules.decompressor.ShmStream
import java.awt.event.WindowAdapter
import java.awt.event.WindowEvent

class InboundStreamWindow(val source: ShmStream, val mgr: DisplayManager) extends JFrame {

    /**
     * ************************************************
     *
     *
     * 			CONSTRUCTION
     *
     * ***********************************************
     */

    class ResizingImgPanel extends Component {
        override def paint(g: Graphics) {
            val g2d = g.asInstanceOf[Graphics2D]
            g2d.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_NEAREST_NEIGHBOR)
            g2d.drawImage(img, 0, 0, this.getWidth(), this.getHeight(), null);
        }
        override def update(g: Graphics) {
            paint(g)
        }
    }

    private var img = ImageIO.read(this.getClass().getResourceAsStream("Koala.jpg")) //new File("imgs/Koala.jpg")) //new BufferedImage(640, 480, BufferedImage.TYPE_4BYTE_ABGR)
    val imgPanel = new ResizingImgPanel
    add(imgPanel);

    setSize(400, 400)
    setLocation(100, 100)
    setVisible(true)
    setDefaultCloseOperation(WindowConstants.DISPOSE_ON_CLOSE)
    setTitle(s"Stream: ${source.streamName}, Source: ${source.callsign} ")
  
    addKeyListener(new KeyAdapter() {
        override def keyPressed(e: KeyEvent) {
            if (e.getExtendedKeyCode() == KeyEvent.VK_ESCAPE) {
                dispose()
            }
        }
    })
    
    addWindowListener(new WindowAdapter() {
        override def windowClosing(e: WindowEvent) {
        	mgr.remove(InboundStreamWindow.this)
        }
    })
    
    private def imgData() = {
        img.getRaster().getDataBuffer().asInstanceOf[DataBufferInt].getData()
    }

    private def ensureSize(w: Int, h: Int) {
        if (img.getWidth() != w || img.getHeight() != h || img.getType() != BufferedImage.TYPE_INT_RGB) {
            img = new BufferedImage(w, h, BufferedImage.TYPE_INT_RGB)
        }
    }

    /**
     * ************************************************
     *
     *
     * 			EXPOSED API
     *
     * ***********************************************
     */

    def paintBytes(f: Array[Int] => Unit, w: Int, h: Int) {
        ensureSize(w, h)
        f(imgData())
        if (imgPanel.isVisible()) {
            val g = imgPanel.getGraphics()
            if (g != null) {
                imgPanel.update(g)
                g.dispose()
            }
        }
    }

    override def toString(): String = {
        s"StreamWindow, ${source.callsign}, ${source.streamName}"
    }

}