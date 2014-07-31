package trainer

import java.io.File
import java.io.FileInputStream
import java.io.FileNotFoundException
import java.io.FileOutputStream
import java.util.Properties
import libgurra.exceptions.NoExcept
import libgurra.misc.SwingErrMsg

object AppConfig extends Properties {

    private val defaultPath = System.getProperty("user.home") + "/trainer/cfg"
    private val defaultFileName = "defaultCfg.xml"

    def defaultFullPath() = s"${defaultPath}/${defaultFileName}"

    def saveToFile(fileName: String) {
        val fullPath = s"${defaultPath}/${fileName}"
        NoExcept.run[Throwable] {
            setProperty("time_local", new java.util.Date().toString())
            val file = new File(fullPath)
            if (!file.exists()) {
                new File(defaultPath).mkdirs()
                file.createNewFile()
            }
            val fOut = new FileOutputStream(file)
            storeToXML(fOut, "")
            fOut.close()
            println(s"Saved configuration to $fullPath")
        }.foreach { ex =>
            SwingErrMsg(s"Failed to store configuration to:\n" +
                s"  ${fullPath}. \n" +
                s"Error: \n" +
                s"  ${ex.getMessage()}")

        }
    }

    def saveToDefaultFile() {
        saveToFile(defaultFileName)
    }

    def loadFromFile(fileName: String) {
        val fullPath = s"${defaultPath}/${fileName}"
        try {
            loadFromXML(new FileInputStream(new File(fullPath)))
            println(s"Loaded configuration from $fullPath")
        } catch {
            case t: FileNotFoundException =>
                SwingErrMsg(s"No configuration file found on system (This may be your first startup). \n" +
                    s"Creating a new configuration file at: \n $fullPath.")
                saveToDefaultFile()
            case t: Throwable =>
                SwingErrMsg(s"Failed to load configuration file $fullPath." +
                    s"Settings won't be able to save on application exit." +
                    s"Ensure access rights to this file and remove and previous corrupt files with this name")
        }
    }

    def getOrUpdate(key: String, f: => String) = {
        val exist = getProperty(key)
        if (exist != null) {
            exist
        } else {
            setProperty(key, f)
            getProperty(key)
        }
    }
    
    def getOrUpdateJ(key: String, default: String) = {
    	getOrUpdate(key, default)
    }

    def loadFromDefaultFile() {
        loadFromFile(defaultFileName)
    }

}