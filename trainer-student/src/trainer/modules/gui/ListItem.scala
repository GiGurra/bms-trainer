package trainer.modules.gui

object ListItem {

    def strings2htmlLines(s: Seq[String]) = {
        s.mkString("<br>")
    }

    def strings2html(s: String*) = {
        "<html>" + s.map(_ + "<br>").mkString + "</html>"
    }

}

abstract class ListItem() {

    def strings2html(s: String*): String = ListItem.strings2html(s: _*)

    def mkTooltip(): String

    def onGuiListRemove()

}