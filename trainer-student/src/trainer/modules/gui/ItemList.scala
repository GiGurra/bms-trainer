package trainer.modules.gui

import java.awt.event.MouseEvent
import java.awt.event.MouseMotionAdapter

import scala.collection.JavaConversions.asScalaBuffer

import javax.swing.DefaultListModel
import javax.swing.JList
import libgurra.math.Clamp

class ItemList[T <: ListItem](model: DefaultListModel[T]) extends JList[T](model) {

    addMouseMotionListener(new MouseMotionAdapter() {
        override def mouseMoved(e: MouseEvent) {
            val p = e.getPoint();
            val i = locationToIndex(p);
            if (i >= 0 && i < model.getSize()) {
                setToolTipText(model.getElementAt(i).mkTooltip());
            } else {
                setToolTipText("No connections");
            }
        }
    });

    def this() = this(new DefaultListModel[T])

    override def getModel() = model

    def ensurePresent(item: T) {
        if (!contains(item)) {
            model.addElement(item);
        }
    }

    def remove(item: T) {
        model.removeElement(item);
    }

    def removeIf(f: T => Boolean) {
        for (i <- (model.getSize() - 1) to 0 by -1) {
            if (f(model.getElementAt(i))) {
                model.remove(i)
            }
        }
    }

    def contains(item: T): Boolean = {
        for (i <- 0 until model.getSize()) {
            if (model.getElementAt(i) == item) {
                return true
            }
        }
        return false
    }

    def selected(): java.util.List[T] = getSelectedValuesList()

    def getNextSelectionIndex(): Option[Int] = {

        val selected: Seq[T] = getSelectedValuesList()

        if (selected.size == 1 && model.size() >= 2) {
            Some(Clamp.apply(0, getSelectedIndex(), model.size() - 2))
        } else {
            None
        }

    }

    def removeSelected() {

        val selected: Seq[T] = getSelectedValuesList()
        val nextSelection = getNextSelectionIndex()

        for (item <- selected) {
            model.removeElement(item);
            item.onGuiListRemove()
        }

        nextSelection.foreach(setSelectedIndex(_))

    }

}