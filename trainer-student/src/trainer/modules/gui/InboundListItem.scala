package trainer.modules.gui

import trainer.modules.decompressor.ShmStream

class InboundListItem(val stream: ShmStream) extends ListItem() {
    override def mkTooltip(): String = ListItem.strings2html(
        "Callsign: ",
        s" * ${stream.callsign}",
        "Stream:",
        s" * ${stream.streamName}")

    def display() {
        stream.openWindow()
    }

    def close() {
        stream.closeWindow()
    }

    override def onGuiListRemove() {
        close()
    }

    override def toString(): String = {
        s"${stream.callsign}:${stream.streamName}"
    }

}