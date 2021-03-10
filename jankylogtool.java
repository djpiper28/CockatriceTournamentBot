import java.io.FileOutputStream;
import java.io.File;
import java.io.IOException;

public class jankylogtool {

    public static void main(String[] args) throws IOException {
        File file = new File("log");
        FileOutputStream f = new FileOutputStream(file);

        int a;
        while ((a = System.in.read()) != -1) {
            System.out.print((char) a);
            f.write(a);
        }
        
        f.close();
    }

}
