import ChatApp.*; 
import org.omg.CosNaming.*; // HelloServer will use the naming service. 
import org.omg.CosNaming.NamingContextPackage.*; // ..for exceptions. 
import org.omg.CORBA.*;     // All CORBA applications need these classes. 
import org.omg.PortableServer.*;   
import org.omg.PortableServer.POA;
import java.util.*;

//TO ADD: GameCallback för server RMIs på client-gameboards

class GameImpl extends GamePOA
{
    private ORB orb;
    private ChatImpl chatImpl;

    private static final int maxX = 8; // W
    private static final int maxY = 8; // H
    
    private Map<String, GameCallback> clients = new HashMap<String, GameCallback>(); //Linking player to client GB
    private Map<String, char> players = new HashMap<String, char>(); //Linking player to a colour
    private char[][] gameBoard = new char[maxX][maxY];
    private boolean[][] legalMoves = new boolean[maxX][maxY];
    private char activeColour;

    //*** Initialization, Construction ***
    public GameImpl(ChatImpl chatImpl)
    {
	this.chatImpl = chatImpl;
	resetgame();
    }


    //*** Methods ***
    public void setORB(ORB orb)
    {
	this.orb = orb;
    }

    public void join(ChatCallback chatref, GameCallback gbref, String nickname, char colour)
    {
	if (clients.containsKey(nickname)) {
	    //Already playing
	    chatref.callback("\u001b[31;1m" + nickname + " is playing already!\u001b[0m");
	}
	else {
	    clients.put(nickname, gbref);
	    players.put(nickname, colour);
	    //Announce
	    for (ChatCallback client : chatImpl.clients.values()) {
		if (client != chatref)
		    client.callback(nickname + " is now playing Othello on team " + colour  + "!");
		else
		    client.callback("Joined Othello.");
	    }   
	    // RMI update/print board!
	}
    }

    public void makemove(ChatCallback chatref, String nickname, String move)
    {
	if (players.get(nickname) != activeColour) {
	    //Wrong turn
	    chatref.callback("It's not your turn.");
	    return;
	}
	
	int x = move[0] - 97; //int(char('a')) == 97
	int y = move[1] - 1;  //1 to 8 --> 0 to 7

	if ( (x<0 || y<0) ||
	    (x>(maxX-1) || y>(maxY-1)) ) {
	    //Out of bounds
	    chatref.callback("Out of bounds.");
	    return;
	}

	if (legalMoves[x][y] == false) {
	    //Illegal move
	    chatref.callback("You can't make that move.");
	    return;
	}

	//Perform move
	gameBoard[x][y] = activeColour;
	flip_affected(x, y); // TO ADD: private flip_affected

	//Turn change
	if (activeColour == 'o')
	    activeColour = 'x';
	else
	    activeColour = 'o';

	calc_legalMoves();

	//Update client gameboards
	/* Put code here */
  
    }

    public void passturn() //TO ADD
    {
    }

    //TO ADD: list()

    public void leave(String nickname)
    {
        clients.remove(nickname);
	players.remove(nickname);
	for (ChatCallback client : chatImpl.clients.values()) {
	    if (client != chatref)
		client.callback(nickname + " stopped playing Othello.");
	    else
		client.callback("You have stopped playing.");
	}
    }
	
    public void resetgame()
    {
	for(char[] column : gameBoard){
	    Arrays.fill(column, ".");
	}
	// place starting pieces
	// set legalmoves
	// set activeColour
    }
    
    private void calc_legalMoves()
    {


    }

}
