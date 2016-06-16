import ChatApp.*;          // The package containing our stubs
import org.omg.CosNaming.*; // HelloClient will use the naming service.
import org.omg.CosNaming.NamingContextPackage.*;
import org.omg.CORBA.*;     // All CORBA applications need these classes.
import org.omg.PortableServer.*;   
import org.omg.PortableServer.POA;
import java.util.*;

 
class ChatCallbackImpl extends ChatCallbackPOA {
    private ORB orb;

    public void setORB(ORB orb_val) {
	orb = orb_val;
    }

    public void callback(String notification){
	System.out.println(notification);
    }
}

public class ChatClient {
    static Chat chatImpl;
    static Game gameImpl;
    
    public static void main(String args[]){
	try {
	    // create and initialize the ORB
	    ORB orb = ORB.init(args, null);

	    // create servant (impl) and register it with the ORB
	    ChatCallbackImpl chatCallbackImpl = new ChatCallbackImpl();
	    chatCallbackImpl.setORB(orb);

	    /* extra bit */
	    GameCallbackImpl gameCallbackImpl = new GameCallbackImpl();
	    gameCallbackImpl.setORB(orb);
	    /* /extra bit */

	    // get reference to RootPOA and activate the POAManager
	    POA rootpoa = 
		POAHelper.narrow(orb.resolve_initial_references("RootPOA"));
	    rootpoa.the_POAManager().activate();
	    
	    // get the root naming context 
	    org.omg.CORBA.Object objRef = 
		orb.resolve_initial_references("NameService");
	    NamingContextExt ncRef = NamingContextExtHelper.narrow(objRef);
	    
	    // resolve the object reference in naming
	    String name = "Chat";
	    chatImpl = ChatHelper.narrow(ncRef.resolve_str(name));
	    String name2 = "Game";
	    gameImpl = GameHelper.narrow(ncRef.resolve_str(name2));

	    // obtain callback reference for registration w/ server
	    org.omg.CORBA.Object ref = 
		rootpoa.servant_to_reference(chatCallbackImpl);
	    ChatCallback cref = ChatCallbackHelper.narrow(ref);

	    /* extra bit */
	    /* /extra bit */
	    ref = rootpoa.servant_to_reference(gameCallbackImpl);
	    GameCallback gref = GameCallbackHelper.narrow(ref);
        
	    // Application code goes below
	    String nickname = "";
	    String[] input;
	    Scanner in = new Scanner(System.in);
	    boolean Active = false;
	    boolean Playing = false;

	    // Welcome Message
	    System.out.println("\u001b[32;1m\n" 
			       + "Welcome to ShitShat!\n"
			       + "Commands available: (shorts available with \\ , i.e. \\j to join)\n\u001b[0m"
			       + "(j)oin <nick>           \u001b[35mJoin chat \u001b[0m\n"
			       + "pos(t) <msg>            \u001b[35mPost to chat \u001b[0m\n"
			       + "(l)ist                  \u001b[35mList connected users \u001b[0m\n"
			       + "pl(a)y <color>          \u001b[35mPlay game \u001b[0m\n"
			       + "passtur(n)              \u001b[35mPass turn in-game\u001b[0m\n"
			       + "(g)list                 \u001b[35mList connected players \u001b[0m\n"
			       + "reset                   \u001b[35mReset the game board\u001b[0m\n"
			       + "lea(v)e                 \u001b[35mLeave chat \u001b[0m\n"
			       + "(p)ut <coordinate XY>   \u001b[35mMake a move \u001b[0m\n"
			       + "(q)uit                  \u001b[35mQuit ShitShat \u001b[0m\n");

	    while(true){
              
		input = in.nextLine().split(" ");

		// Join
		if (input[0].equals("join") || input[0].equals("\\j")){
		    if (input.length < 2){
			System.out.println("\u001b[31;1m No name given at command line!\u001b[0m");
		    }
		    else if (!Active){
			nickname = chatImpl.join(cref, input[1]);
			if (!(nickname.equals("active"))){
			    Active = true;
			}
		    }
		    else{
			System.out.println("\u001b[31;1mDon't join twice, " + nickname + "\u001b[0m");
		    }
		}

		// Post
		if (input[0].equals("post") || input[0].equals("\\t")){
		    if (Active){
			String msg = "";
			for(int i= 1; i < input.length; i++) {
			    msg = msg + " " + input[i];
			}       
			chatImpl.post(cref, nickname, msg);
		    }
		    else{
			System.out.println("\u001b[31;1mGo active first!\u001b[0m");
		    }
		}

		// List
		if (input[0].equals("list") || input[0].equals("\\l")){
		    chatImpl.list(cref, nickname );
		}

		// List (game)
		if (input[0].equals("glist") || input[0].equals("\\g")){
		    gameImpl.list(cref);
		}

		// Leave (game)
		if (input[0].equals("leave") || input[0].equals("\\v")){
		    if (Active){
			chatImpl.leave(cref, nickname);
			Active = false;

			if (Playing){
			    gameImpl.leave(cref, nickname);
			    //chatImpl.leaveGame(nickname); 
			    Playing = false;
			}
		    }
		    else{
			System.out.println("\u001b[31;1mJoin before leaving!\u001b[0m");
		    }
		}     

		// Play (game)
		if(input[0].equals("play") || input[0].equals("\\a")){
		    if (Active){
			if (input.length > 1){
			    char color = input[1].charAt(0);
			    //chatImpl.play(cref, nickname, color);
			    gameImpl.join(cref, gref, nickname, color);
			    Playing = true;
			}
		    }
		    else{
			System.out.println("\u001b[31;1mJoin first omg\u001b[0m");
		    }
		}

		// Put (game)
		//Note-to-self: int (char(a)) == 97
		if (input[0].equals("put") || input[0].equals("\\p")){
		    if (Playing){
			if (input.length > 1){
			    String pos = input[1];
			    if (pos.matches("([a-h]|[A-H])+([1-8])")){ // Om vi bara vill tillåta a-h, ta bort |[A-H]
				//chatImpl.put(cref, nickname, pos);
				gameImpl.makemove(cref, nickname, pos);
			    }
			    else{
				System.out.println("\u001b[31;1mSyntax: \"put [a-h][1-8]\", i.e. \"put a3\"\u001b[0m");
			    }
			}
			else{
			    System.out.println("Enter a 2-digit coordinate to put");
			}
		    }
		    else{
			System.out.println("\u001b[31;1mJoin a game first\u001b[0m");
		    }
		}

		//Pass Turn
		if (input[0].equals("passturn") || input[0].equals("\\n")) {
		    System.out.println("\u001b[36mPassed turn!\u001b[0m");
		    gameImpl.passturn();
		}

		//Reset
		if (input[0].equals("reset")) {
		    boolean manReset = true;
		    gameImpl.reset(manReset);
		}
		
		// Quit
		if (input[0].equals("quit") || input[0].equals("\\q")){
		    if (Active){
			chatImpl.leave(cref, nickname);
			Active = false;
			System.out.println("\u001b[31;1mStill active, leaving ...\u001b[0m");
		    }

		    System.out.println("\u001b[36mBye " + nickname + "!\u001b[0m");
		    System.exit(0);
		} 
	    }              
	}
	catch(Exception e){
	    System.out.println("\u001b[31;1mERROR : " + e + "\u001b[0m");
	    e.printStackTrace(System.out);
	}
    }
}
