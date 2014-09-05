/*  
 *  Copyright(C) 2014, Jérôme Benoit <jerome.benoit@piment-noir.org> under GPLv3 
 *  Debug.pde
 *  A very basic debug facility for processing
 *  Usage : Create a debug class inside your sketch and you will 
 *  have string print once functionnality inside the main processing draw() loop.
 *  Hook properly the last call to prtStr method inside the draw() loop.   
 */

public static final class Debug {
    private static boolean enableDebug = false;
    private static int printCount = 0; 
     
    public Debug(boolean enableDebug) {
      this.enableDebug = enableDebug; 
    }  
       
    public void DonePrinting() {
      printCount = 1;
    }

    public void UndoPrinting() {
      if (enableDebug) printCount = 0; 
    }
  
    public void prStr(String string) {
      if (printCount == 0 && enableDebug) { 
      println(string);
      }   
    }
}

