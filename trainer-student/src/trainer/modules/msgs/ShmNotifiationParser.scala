package trainer.modules.msgs

import java.nio.ByteBuffer

import libgurra.parsing.MessageParser

class ShmNotifiationParser extends MessageParser[ShmNotificationMsg](10 * 1024) {

    override def testBuffer(buffer: ByteBuffer): Either[ShmNotificationMsg, MessageParser.ParseTestErr] = {

        val initialBufferPos = buffer.position();

        if (buffer.remaining() >= 8) {
            val magicNbr = buffer.getInt()
            val strLen = buffer.getInt()
            if (magicNbr != 123456789) {
                buffer.position(initialBufferPos)
                Right(MessageParser.NOT_A_MESSAGE)
            } else if (strLen <= buffer.remaining()) {
                val stringBytes = new Array[Byte](strLen)
                buffer.get(stringBytes)
                buffer.position(initialBufferPos)
                Left(new ShmNotificationMsg(stringBytes))
            } else {
                buffer.position(initialBufferPos)
                Right(MessageParser.NOT_ENOUGH_BYTES)
            }
        } else {
            Right(MessageParser.NOT_ENOUGH_BYTES)
        }

    }
}