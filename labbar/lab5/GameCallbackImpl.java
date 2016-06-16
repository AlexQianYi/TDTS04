import ChatApp.*; 
import org.omg.CosNaming.*; // HelloServer will use the naming service. 
import org.omg.CosNaming.NamingContextPackage.*; // ..for exceptions. 
import org.omg.CORBA.*;     // All CORBA applications need these classes. 
import org.omg.PortableServer.*;   
import org.omg.PortableServer.POA;
import java.util.*;


class GameCallbackImpl extends GameCallbackPOA
{
    private ORB orb;

    private static final int maxX = 8; // W
    private static final int maxY = 8; // H
    
    private GuiTextArea gbGUI;
    private char[][] mirrored_gameBoard = new char[maxX][maxY];

    //*** Methods ***
    public void setORB(ORB orb)
    {
	this.orb = orb;
    }

    public void setGUI(GuiTextArea gbgui)
    {
	this.gbGUI = gbgui;
    }

    public void startgame(String nickname, char colour)
    {
	gbGUI = new GuiTextArea("Othello - " + nickname + " on team " + colour);
    }
    
    public void closegame()
    {
	gbGUI = null;
    }

    public void boardupdate(String gbstate)
    {
	for (int i = 0 ; i < maxX ; ++i) {
	    for (int j = 0 ; j < maxY ; ++j) {
		mirrored_gameBoard[i][j] = gbstate.charAt(j + i*8);
	    }
	}
	renderboard();
    }

    private void renderboard()
    {
	gbGUI.clear();
	gbGUI.println("  | a b c d e f g h |");
	gbGUI.println("__|_________________|__");

	for (int j = 0 ; j < maxY ; ++j) {
	    gbGUI.print((j+1) + " | ");
	    for (int i = 0 ; i < maxX ; ++i) {
		gbGUI.print(mirrored_gameBoard[i][j] + " ");
	    }
	    gbGUI.println("|");
	}

	gbGUI.println("__|_________________|__");
	gbGUI.println("  |                 |  ");
    }
}
