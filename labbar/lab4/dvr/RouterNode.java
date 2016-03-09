/*
 * 1. Constructor
 * 2. recvUpdate()
 * 3. printDistanceTable()
 *
 * 4. updateLinkCost()
 *
 * 5. Poisoned Reverse implementation
 */


import javax.swing.*;        

class RouteEntry {
    int firsthop;
    int distance;
}

public class RouterNode {
    private int myID;
    private GuiTextArea myGUI;
    private RouterSimulator sim;
    private int[] linkcosts = new int[RouterSimulator.NUM_NODES];
    private int[][] bestDistanceto_ofNeighbour = new int[RouterSimulator.NUM_NODES][RouterSimulator.NUM_NODES]; // Allocating (sizeof dist(int) * NUM_NODES * NUM_NODES) space to this table, instead of (s_o dist(int) * NN * number of neighbours * s_o what neighbour(int)).
    private RouteEntry[] bestRouteto = new RouteEntry[RouterSimulator.NUM_NODES];
    private final bool[] isNeighbour = new bool[RouterSimulator.NUM_NODES]; // Assuming no severed connections or restructuring
    
    //--------------------------------------------------
    public RouterNode(int ID, RouterSimulator sim, int[] linkcosts) {
	myID = ID;
	this.sim = sim;
	myGUI = new GuiTextArea("  Output window for Router #"+ ID + "  ");
	
	System.arraycopy(linkcosts, 0, this.linkcosts, 0, RouterSimulator.NUM_NODES);
	
	// Set initial best routes and determine neighbours. "Everything in java defaults to 0".
	for (int i ; i < RouterSimulator.NUM_NODES ; ++i) {
	    bestRouteto[i].firsthop = i;
	    if ((bestRouteto[i].distance = linkcosts[i]) != RouterSimulator.INFINITY)
		isNeighbour[i] = true;
	    else // *** <- Don't need? ***
		isNeighbour[i] = false;
	}

    }
    
    //--------------------------------------------------
    public void recvUpdate(RouterPacket pkt) {
	bool bRtchanged; // = false
	int src {pkt.sourceid};

	//update bestDistanceto_ofNeighbour[i][sourceid]
	for (int i ; i < RouterSimulator.NUM_NODES ; ++i) {	    
	    bestDistanceto_ofNeighbour[i][src] = pkt.mincost[i]; // Take note of use of the 2-d array of bDt_oN
	    // Update D_thisnode(i) if c(thisnode, src) + D_src(i) < D_thisnode(i) 
	    if (linkcosts[src] + pkt.mincost[i] < bestRouteto[i].distance) {
		bestRouteto[i].distance = linkcosts[src] + pkt.mincost[i];
		bestRouteto[i].firsthop = src;
		bRtchanged = true;
	    }
	// Need to consider update for worse bestRoute
	//  if(bestRouteto[i].firsthop==src)
	// true (if worse)
	// min(D_j(i)+c(me,j), j:0,1,...,NUM_NODES)
	}

	if (bRtchanged) {
	    int[RouterSimulator.NUM_NODES] mincosts;
	    for (int i ; i < RouterSimulator.NUM_NODES ; ++i)
		mincosts[i] = bestRouteto[i].distance;

	    RouterPacket updatepkt(myID, 0, mincosts);

	    for (int i ; i < RouterSimulator.NUM_NODES ; ++i) {
		if(isNeighbour[i]) {
		    updatepkt.destid = i;
		    sendUpdate(updatepkt);
		    //CONTINUE CHECKING FROM HERE
		}
	    }

	}
    }
    
    
    //--------------------------------------------------
    private void sendUpdate(RouterPacket pkt) {
	sim.toLayer2(pkt);
	
    }
  
    
    //--------------------------------------------------
    public void printDistanceTable() {
	myGUI.println("Current table for " + myID +
		      "  at time " + sim.getClocktime());
    }

    //--------------------------------------------------
    public void updateLinkCost(int dest, int newcost) {
	// Särskilj rutt och länk!!
	//Länk + rutt < rutt?

    }

}
