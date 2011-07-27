/*
 * Blockette1000.java
 * * The blockette 1000 has :
 * 0  i*2 blockette type  is 1000
 * 2  i*2 Next Blockette byte number (offset to next blockette)
 * 4  Encoding format - 10 = steimI, 11=steim2, 
 * 5  word order 0 little endian, 1=big endian
 * 6  record length power (2^b(6) = length) 
 * 7 reserved

 * Created on November 21, 2006, 3:30 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package seed;
import java.nio.ByteBuffer;

/** This class represents the Blockette 1000 from the SEED standard V2.4 
 * Blockette1000.java
 * * The blockette 1000 has :
 * 0  i*2 blockette type  is 1000
 * 2  i*2 Next Blockette byte number (offset to next blockette)
 * 4  Encoding format - 10 = steimI, 11=steim2, 
 * 5  word order 0 little endian, 1=big endian
 * 6  record length power (2^b(6) = length) 
 * 7 reserved

 *
 * @author davidketchum
 */
public class Blockette1000 {
  
  byte [] buf;
  ByteBuffer bb;
  /** Creates a new instance of Blockette1000
   * @param b The block of 8 bytes to build this blockette 1000 from*/
  public Blockette1000(byte [] b) {
    buf = new byte[b.length];
    bb = ByteBuffer.wrap(buf);
    System.arraycopy(b,0, buf, 0, b.length);
  }
  public void reload(byte [] b) {System.arraycopy(b, 0, buf, 0, b.length);}
  /** get the 8 raw bytes
   *@return The 8 raw bytes in theis 1000 block */
  public byte [] getBytes() {return buf;}
  /** get encoding
   *@return the encodeing 10-steim1, 11=steim2, 19=steim3*/
  public int getEncoding() {return ((int) buf[4]) & 0xff;}
  /** get record length is bytes
   *@return The record length in bytes (512, 4096, etc)*/
  public int getRecordLength() {return 1<< (int) buf[6];}
  /** get word order
   * @return true if bigEndian, false if little endian 
   */
  public boolean isBigEndian() {return (buf[5] != 0);}
  /** set the encoding
   *@param b The encoding to use (10-steim1, 11=steim2, 19=steim3*/
  public void setEncoding(byte b) {buf[4] = b;}
  public void setNextOffset(int i) {bb.position(2); bb.putShort((short) i);}
  public short getNextOffset() {bb.position(2); return bb.getShort();}
  /** string rep
   *@return A string documenting this b1000 contents*/
  @Override
  public String toString() {return "(M:e="+getEncoding()+" l="+getRecordLength()+(isBigEndian()?"":"S")+")";}
  /** set the record length
   *@param len the record length in bytes */
  public void setRecordLength(int len) {
    int i = 0;
    len = len >> 1;
    while(len > 0) {
      i++;
      len = len >> 1;
    }
    buf[6] = (byte) i;
  }

}
