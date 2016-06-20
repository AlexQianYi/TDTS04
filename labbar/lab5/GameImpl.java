import ChatApp.*; 
import org.omg.CosNaming.*; // HelloServer will use the naming service. 
import org.omg.CosNaming.NamingContextPackage.*; // ..for exceptions. 
import org.omg.CORBA.*;     // All CORBA applications need these classes. 
import org.omg.PortableServer.*;   
import org.omg.PortableServer.POA;
import java.util.*;


class GameImpl extends GamePOA
{
    private ORB orb;
    private ChatImpl chatImpl;

    private static final int maxX = 8; // W
    private static final int maxY = 8; // H
    
    private Map<String, GameCallback> clients = new HashMap<String, GameCallback>(); //Linking player to client GB
    private Map<String, String> players = new HashMap<String, String>(); //Linking player to a colour
    private char[][] gameBoard = new char[maxX][maxY];
    private boolean[][] legalMoves = new boolean[maxX][maxY];
    private char activeColour;
    private char opposingColour;

    //*** Initialization, Construction ***
    public GameImpl(ChatImpl chatImpl)
    {
	this.chatImpl = chatImpl;
	
	boolean manReset = false;
	reset(manReset);
    }


    //*** Methods ***
    public void setORB(ORB orb)
    {
	this.orb = orb;
    }

    public boolean join(ChatCallback chatref, GameCallback gbref, String nickname, char colour)
    {
	if (clients.containsKey(nickname)) {
	    //Already playing
	    chatref.callback("\u001b[31;1m" + nickname + " is playing already!\u001b[0m");
	    return false;
	}
	else {
	    String colourString = String.valueOf(colour);

	    System.out.println(colourString);

	    clients.put(nickname, gbref);
	    players.put(nickname, colourString);
	    //Announce
	    for (ChatCallback client : chatImpl.clients.values()) {
		if (client != chatref)
		    client.callback(nickname + " is now playing Othello on team " + colourString + "!");
		else
		    client.callback("Joined Othello.");
	    }
	    //Send gameboard data to player 
	    gbref.startgame(nickname, colour);
	    gbref.boardupdate( gbStringify() );
	    return true;
	}
    }

    public boolean makemove(ChatCallback chatref, String nickname, String move)
    {
	System.out.println("makemove1");

	System.out.println(players.get(nickname));

	if (players.get(nickname).charAt(0) != activeColour) {
	    chatref.callback("It's not your turn.");
	    return false;
	}

       	System.out.println("makemove2");

	int x = move.charAt(0) - 97; //int(char('a')) == 97
	int y = move.charAt(1) - 49;  //1 to 8 --> 0 to 7

	if (!inbounds(x, y)) {
	    chatref.callback("Out of bounds.");
	    return false;
	}

	System.out.println("makemove3");

	if (legalMoves[x][y] == false) {
	    chatref.callback("You can't make that move.");
	    return false;
	}

	//Perform move
	gameBoard[x][y] = activeColour;
	flip_affected(x, y);

	//Turn change
	turn_change_ao(opposingColour, activeColour);

	//Calculate legal moves for next turn.
	calc_legalMoves();

	//Update client gameboards
	for (GameCallback gbref : clients.values()) {
	    gbref.boardupdate( gbStringify() );
	}
  
	return true;
    }

    public void passturn()
    {
	//Turn change
	turn_change_ao(opposingColour, activeColour);

	//Calculate legal moves for next turn.
	calc_legalMoves();
    }

    public void list(ChatCallback chatref)
    {	
	Set<String> playersSet = players.keySet();
	Iterator<String> it = playersSet.iterator();
	Vector<String> teamx = new Vector();
	Vector<String> teamo = new Vector();
	String temp;
	
	while (it.hasNext()) {
	    temp = it.next();
	    if (players.get(temp).charAt(0) == 'x')
		teamx.add(temp);
	    else
		teamo.add(temp);
	}

	chatref.callback(players.size() + " players playing Othello.");
	chatref.callback("Team x:");
	for (String player : teamx)
	    chatref.callback(player);

	chatref.callback("Team o:");
	for (String player : teamo)
	    chatref.callback(player);	
    }

    public void leave(ChatCallback chatref, GameCallback gbref, String nickname)
    {
	gbref.closegame();
        clients.remove(nickname);
	players.remove(nickname);
	for (ChatCallback client : chatImpl.clients.values()) {
	    if (client != chatref)
		client.callback(nickname + " stopped playing Othello.");
	    else
		client.callback("You have stopped playing.");
	}
    }
	
    public void reset(boolean manReset)
    {
	if (manReset == true) {
	    for (ChatCallback client : chatImpl.clients.values()) {
		    client.callback("\u001b[31;1m The gameboard was manually reset.\u001b[0m");
	    }
	}
	//Reset pieces.
	for (char[] column : gameBoard){
	    Arrays.fill(column, '.');
	}
	gameBoard[3][3] = 'x';
	gameBoard[3][4] = 'o';
	gameBoard[4][3] = 'o';
	gameBoard[4][4] = 'x';
 
	//x is first to act. 'x' and 'o' becomes default colours. 
	turn_change_ao('x', 'o');

	//Calculate legal moves for next turn.
	calc_legalMoves();

	//Update client gameboards
	for (GameCallback gbref : clients.values()) {
	    gbref.boardupdate( gbStringify() );
	}
    }
    
    private String gbStringify()
    {
	String retString = new String();

	for (int i = 0 ; i < maxX ; ++i) {
	    for (int j = 0 ; j < maxY ; ++j) {
		retString = retString + gameBoard[i][j];
	    }
	}
	return retString;
    }

    private boolean inbounds(int x, int y)
    {
	return (x >= 0 &&
		y >= 0 &&
		x < maxX &&
		y < maxY);	
    }

    private void turn_change_ao(char newactive, char newopposing)
    {
	activeColour = newactive;
	opposingColour = newopposing;
    }

    private void calc_legalMoves()
    {
	for (int x = 0 ; x < maxX ; ++x) {
	    for (int y = 0; y < maxY ; ++y) {
		//(x,y) is illegal until we prove it's not.
		legalMoves[x][y] = false;
		
		//Check that (x,y) is empty
		if (gameBoard[x][y] != '.')
		    continue;

		//Check all immediate neighbouring squares (x+i,y+j).
		outerloop:
		for (int i = -1 ; i < 2 ; ++i) {
		    for (int j = -1 ; j < 2 ; ++j) {
			if (inbounds(x+i, y+j) && !(i == 0 && j == 0)) {
			    if (gameBoard[x+i][y+j] == opposingColour) {
				//Opposing piece found in neighbouring square, direction (i,j).
				for (int n = 1 ; inbounds(x+n*i, y+n*j) ; ++n) {
				    //Check if consecutive line of opposing piece can be made in direction (i,j) to a friendly piece.
				    if (gameBoard[x+n*i][y+n*j] == opposingColour)
					continue;
				    if (gameBoard[x+n*i][y+n*j] == '.')
					break;
				    if (gameBoard[x+n*i][y+n*j] == activeColour) {
					//Sequence of o x ... x o or vice versa found. (x,y) is a legal move.
					legalMoves[x][y] = true;
					break outerloop;
				    }
				}
		
			    }		
			}
		    }
		}
	    }
	}
    }

    private void flip_affected(int x, int y)
    {
	//Check all immediate neighbouring squares (x+i,y+j).
	for (int i = -1 ; i < 2 ; ++i) {
	    for (int j = -1 ; j < 2 ; ++j) {
		if (inbounds(x+i, y+j)) {
		    if (gameBoard[x+i][y+j] == opposingColour) {
			//Opposing piece found in neighbouring square, direction (i,j).
			for (int n = 1 ; inbounds(x+n*i, y+n*j) ; ++n) {
			    //Check if consecutive line of opposing piece can be made in direction (i,j) to a friendly piece.
			    if (gameBoard[x+n*i][y+n*j] == opposingColour)
				continue;
			    if (gameBoard[x+n*i][y+n*j] == '.')
				break;
			    if (gameBoard[x+n*i][y+n*j] == activeColour) {
				//Sequence of o x ... x o or vice versa found. Flip pieces.
				for (int m = 1 ; m < n ; ++m) {
				    gameBoard[x+m*i][y+m*j] = activeColour;
				}
				break;
			    }
			}
		
		    }		
		}
	    }
	}
    }

}
