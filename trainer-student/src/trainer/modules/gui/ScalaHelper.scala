package trainer.modules.gui

import trainer.modules.decompressor.ShmStream

object ScalaHelper {
    def removeListItem(stream: ShmStream, from: ItemList[InboundListItem]) {
        from.removeIf(_.stream == stream)
    }
}