package trainer.modules.msgs

import java.nio.ByteBuffer

import libgurra.parsing.MessageBase

object StreamPacketHeader {
    val SIZE = 92
    val MAGIC_NUMBER = 123456789
    val NAME_LEN = 32
}

object StreamPacket {

    def parseHeader(headerBytes: Array[Byte]): StreamPacketHeader = {
        val src = ByteBuffer.wrap(headerBytes)
        val callsignBytes = new Array[Byte](StreamPacketHeader.NAME_LEN)
        val streamNameBytes = new Array[Byte](StreamPacketHeader.NAME_LEN)

        val magicNbr = src.getInt()
        val wholeMsgSize = src.getInt()
        val width = src.getInt()
        val height = src.getInt()
        val payloadSize = src.getInt()
        val frameNbr = src.getInt()
        src.get(callsignBytes)
        src.get(streamNameBytes)
        val callsign = new String(callsignBytes).trim()
        val streamName = new String(streamNameBytes).trim()
        val zPad = src.getInt()

        new StreamPacketHeader(
            headerBytes,
            magicNbr,
            wholeMsgSize,
            width,
            height,
            payloadSize,
            frameNbr,
            callsign,
            streamName,
            zPad)
    }

    def parseFrom(allMsgBytes: Array[Byte]): StreamPacket = {
        val src = ByteBuffer.wrap(allMsgBytes)
        val callsignBytes = new Array[Byte](StreamPacketHeader.NAME_LEN)
        val streamNameBytes = new Array[Byte](StreamPacketHeader.NAME_LEN)

        val magicNbr = src.getInt()
        val wholeMsgSize = src.getInt()
        val width = src.getInt()
        val height = src.getInt()
        val payloadSize = src.getInt()
        val frameNbr = src.getInt()
        src.get(callsignBytes)
        src.get(streamNameBytes)
        val callsign = new String(callsignBytes).trim()
        val streamName = new String(streamNameBytes).trim()
        val zPad = src.getInt()
        val dataBytes = new Array[Byte](payloadSize)
        src.get(dataBytes)

        val header = new StreamPacketHeader(
            allMsgBytes,
            magicNbr,
            wholeMsgSize,
            width,
            height,
            payloadSize,
            frameNbr,
            callsign,
            streamName,
            zPad)

        new StreamPacket(header, dataBytes)
    }

}

case class StreamPacketHeader(
    val allHeaderBytes: Array[Byte],
    val magicNbr: Int,
    val wholeMsgSize: Int,
    val width: Int,
    val height: Int,
    val payloadSize: Int,
    val frameNbr: Int,
    val callsign: String,
    val streamName: String,
    val zPad: Int) extends MessageBase {

    override def bytes(): Array[Byte] = {
        allHeaderBytes
    }

    override def size(): Int = {
        allHeaderBytes.length
    }

}

case class StreamPacket(
    val header: StreamPacketHeader,
    val dataBytes: Array[Byte]) extends MessageBase {

    override def bytes(): Array[Byte] = {
        List(header.allHeaderBytes, dataBytes).flatten.toArray
    }

    override def size(): Int = {
        header.allHeaderBytes.length + dataBytes.length
    }

}
