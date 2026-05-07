package file;

import entities.DoubleList;

import java.io.FileWriter;
import java.io.IOException;
import java.util.Collection;

public class Output {

    public static void output(String path, Collection<?> content) {
        try {
            FileWriter fw = new FileWriter(path);
            for (Object l : content) {
                DoubleList<?>.Iterator iter = ((DoubleList<?>) l).iterator();
                while (iter.hasNext()) {
                    fw.write(iter.next().toString() + "\n");
                }
                fw.write("\n");
            }
            fw.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

}
