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
    
    public static void main(String args[]){
	try {
	    // create and initialize the ORB
	    ORB orb = ORB.init(args, null);

	    // create servant (impl) and register it with the ORB
	    ChatCallbackImpl chatCallbackImpl = new ChatCallbackImpl();
	    chatCallbackImpl.setORB(orb);

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
	    
	    // obtain callback reference for registration w/ server
	    org.omg.CORBA.Object ref = 
		rootpoa.servant_to_reference(chatCallbackImpl);
	    ChatCallback cref = ChatCallbackHelper.narrow(ref);
        
	    // Application code goes below
	    String nickname = "";
	    String[] input;
	    Scanner in = new Scanner(System.in);
	    boolean Active = false;
	    boolean Playing = false;

	    System.out.println("\u001b[32;1m\n" 
			       + "Welcome to ShitShat!\n"
			       + "Commands available:\n\u001b[0m"
			       + "join\n"
			       + "post\n"
			       + "list\n"
			       + "play\n" 
			       + "put\n"
			       + "quit\n");
          
	    while(true){
              
		input = in.nextLine().split(" ");

		if (input[0].equals("join")){
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
    
		if (input[0].equals("post")){
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
        
		if (input[0].equals("list")){
		    chatImpl.list(cref, nickname );
		}
        
		if (input[0].equals("leave")){
		    if (Active){
			chatImpl.leave(cref, nickname);
			Active = false;

			if (Playing){
			    chatImpl.leaveGame(nickname); 
			    Playing = false;
			}
		    }
		    else{
			System.out.println("\u001b[31;1mJoin before leaving!\u001b[0m");
		    }
		}     

		if(input[0].equals("play")){
		    if (Active){
			if (input.length > 1){
			    String color = input[1].substring(0,1);
			    chatImpl.play(cref, nickname, color);
			    Playing = true;
			}
		    }
		    else{
			System.out.println("\u001b[31;1mJoin first omg\u001b[0m");
		    }
		}
		//Note-to-self: int (char(a)) == 97
		if (input[0].equals("put")){
		    if (Playing){
			if (input.length > 1){
			    String pos = input[1];
			    if (pos.matches("([a-h]|[A-H])+([1-8])")){
				chatImpl.put(cref, nickname, pos);
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

		if (input[0].equals("quit")){
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
