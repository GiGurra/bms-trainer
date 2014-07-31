package trainer.modules.gui

class Gui {

    val mainGui = new TrainerGui
    mainGui.setVisible(true)
    
    val displayManager = new DisplayManager

    def isVisible() = mainGui.isVisible()

}