import ChatApp.*; 
import java.util.*;

class Game{
  private static int maxX = 8; // W
  private static int maxY = 8; // H
  // Players?
  public Map<String, ChatCallback> players = new HashMap<String, ChatCallback>();
  // Colors?
  public Map<String, String> colors = new HashMap<String, String>();
  // Board!
  public String[][] board = new String[maxX][maxY];

  public Game(){
    reset();
  }
  
  public void join(ChatCallback objref, String nickname, String color)
  {
    if(players.containsKey(nickname)){
      objref.callback ("\u001b[31;1m" + nickname + " is playing already!\u001b[0m");
      return;
    }
    for (ChatCallback callback : players.values()) // Announce :p
    {
      callback.callback(nickname + " is now playing!");
    }   
    objref.callback("Welcome " + nickname + "!");
    players.put(nickname, objref); // Add new player
    colors.put(nickname, color);  // Add color
  
    // Print board!
    //    objref.callback(somethingomg());
  }
  public void put(ChatCallback objref, String nickname, String pos)
  {
    String color = "";
    int inX, inY;    
  }

  public void leave(String nickname){
    players.remove(nickname);
    colors.remove(nickname);
  }

  private void reset(){
    for(String[] row: board){
      Arrays.fill(row, " ");
    }
  }

}
