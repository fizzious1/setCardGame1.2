package bigbrother.set.ex;

import bigbrother.set.Env;

import java.util.*;

import static java.lang.Thread.sleep;

/**
 * This class manages the players' threads and data
 *
 * @inv id >= 0
 * @inv score >= 0
 */
public class Player implements Runnable {

    /**
     * The game environment object.
     */
    private final Env env;

    /**
     * Game entities.
     */
    private final Table table;

    /**
     * The id of the player (starting from 0).
     */
    public final int id;

    /**
     * The thread representing the current player.
     */
    private Thread playerThread;

    /**
     * The thread of the AI (computer) player (an additional thread used to generate key presses).
     */
    private Thread aiThread;

    /**
     * True iff the player is human (not a computer player).
     */
    private final boolean human;

    /**
     * True iff game should be terminated due to an external event.
     */
    private volatile boolean terminate;

    /**
     * The current score of the player.
     */
    private int score;

    //Added
    List<Character> keyList ;

    Queue<Integer> keysPressed = new LinkedList<>();





    /**
     * The class constructor.
     *
     * @param env    - the game environment object.
     * @param table  - the table object.
     * @param dealer - the dealer object.
     * @param id     - the id of the player.
     * @param human  - true iff the player is a human player (i.e. input is provided manually, via the keyboard).
     */
    public Player(Env env, Dealer dealer, Table table, int id, boolean human) { //TODO CHANGING CONSTRUCTOR NOT ALLOWED
        this.env = env;
        this.table = table;
        this.id = id;
        this.human = human;
        keyList = id==1?
                new ArrayList<Character>() {{
                    add('q');
                    add('w');
                } }:
                new ArrayList<Character>() {{
                    add('u');
                    add('i');
                } };
    }

    /**
     * The main player thread of each player starts here (main loop for the player thread).
     */
    @Override
    public void run() {
        playerThread = Thread.currentThread();
        System.out.printf("Info: Thread %s starting.%n", Thread.currentThread().getName());
        if (!human) createArtificialIntelligence();

        while (!terminate) {
            // TODO implement main player loop
//            System.out.println("Player " +id +": My pressed keys are :" +keysPressed.toString());
//            if(keysPressed.size() ==3)
//                synchronized (this) {
//                    notifyAll();
//                }

        }
        if (!human) try { aiThread.join(); } catch (InterruptedException ignored) {}
        System.out.printf("Info: Thread %s terminated.%n", Thread.currentThread().getName());
    }

    /**
     * Creates an additional thread for an AI (computer) player. The main loop of this thread repeatedly generates
     * key presses. If the queue of key presses is full, the thread waits until it is not full.
     */
    private void createArtificialIntelligence() {
        // note: this is a very, very smart AI (!)
        aiThread = new Thread(() -> {
            System.out.printf("Info: Thread %s starting.%n", Thread.currentThread().getName());
            while (!terminate) {
                // TODO implement player key press simulator
                    Character keyToPress = keyList.get((int) ((Math.random() * keyList.size())));
                    //TODO Consider implementing using playerKeys array in config
                    //TODO Slot number is column +totalColumns*row

                try {
                    synchronized (this) { wait(); }
                } catch (InterruptedException ignored) {}
            }
            System.out.printf("Info: Thread %s terminated.%n", Thread.currentThread().getName());
        }, "computer-" + id);
        aiThread.start();
    }

    /**
     * Called when the game should be terminated due to an external event.
     */
    public void terminate() {
        // TODO implement
    }

    /**
     * This method is called when a key is pressed.
     *
     * @param slot - the slot corresponding to the key pressed.
     */
    public void keyPressed(int slot) {//slot) {
        // TODO implement

        //a key was received from input manger. Store it in a queue.
        if( keysPressed.size() < 3){
            keysPressed.add(slot);
            table.placeToken(id,slot);
            System.out.println("Player::keyPressed : placing token on slot " + slot);

            }
        if(keysPressed.size() ==3) {
            synchronized (this) {
                notifyAll();
            }}
//        if(keysPressed.size()==3){
//
//            //TODO : After placing 3 tokens, notify the dealer thread that you've made your choice and wait until he wakes you up.
//            table.placeToken(id,slot);
//            try{
//                sleep(100);
//            }
//            catch(Exception e ){
//                System.out.println("Tried to sleep but failed.");
//            }
//            int[] setChosen = new int[3] ;
//            for(int i=0; i<3; i++) {
//                setChosen[i] = keysPressed.remove();
//                table.env.ui.removeToken(id, setChosen[i]);
//            }
//
//            if(env.util.testSet(table.slotsToCards(setChosen))) {
//                System.out.println("Set chosen is valid");
//                synchronized (this) {
//                    point();
//                }
//            }
//            else {
//                System.out.println("Bad set.");
//                synchronized (this) {
//                    penalty();
//                }
//            }
//
//            }

        }






    /**
     * Award a point to a player and perform other related actions.
     *
     * @post - the player's score is increased by 1.
     * @post - the player's score is updated in the ui.
     */
    public void point() {
        // TODO implement

        int ignored = table.countCards(); // this part is just for demonstration in the unit tests
        env.ui.setScore(id, ++score);
        try {
            synchronized (this) {
                sleep(env.config.pointFreezeMillis);
            }
        }
        catch (InterruptedException e ){
            System.out.println("Player::point : Tried to reward player number " +id + " but failed.");
        }
    }

    /**
     * Penalize a player and perform other related actions.
     */
    public void penalty() {
        // TODO implement
        try {
            sleep(env.config.penaltyFreezeMillis);
        }
        catch (InterruptedException e ){
            System.out.println("Player::point : Tried to penalize player number " +id + " but failed.");
        }
    }

    public int getScore() {
        return score;
    }



    public synchronized Optional<Integer[]> checkPlayerStatus() throws InterruptedException {
        Optional<Integer[]> setChosen = Optional.empty();
        if(keysPressed.size() == 3) {
            Integer[] setChosenArray = new Integer[3];
            for (int i = 0; i < 3; i++)
                setChosenArray[i] = keysPressed.remove();
            setChosen = Optional.of(setChosenArray);

        }
        return setChosen;
    }
}
