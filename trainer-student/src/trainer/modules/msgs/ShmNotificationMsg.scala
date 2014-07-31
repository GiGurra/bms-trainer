package trainer.modules.msgs

import libgurra.parsing.MessageBase

class ShmNotificationMsg(arr: Array[Byte]) extends MessageBase {
    val shmName = new String(arr)
    def bytes(): Array[Byte] = arr
    def size(): Int = arr.length + 8
}