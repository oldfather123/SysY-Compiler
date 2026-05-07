import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Objects;

public class TestRISCV {
    public static void main(String[] args) throws IOException, InterruptedException {
//        String testPath = "functional";
        String testPath = "functional";
//        String testPath = "performance";
        File directory = new File(testPath);
        List<File> fileList;
        int timeOut = 180;

        // 获取文件列表并过滤
        fileList = new ArrayList<>();
        for(File file : Objects.requireNonNull(directory.listFiles())) {
            if(file.isFile() && file.getName().contains(".sy")) {
                fileList.add(file);
            }
        }
        // 按文件名排序
        fileList.sort(Comparator.comparing(File::getName));
        for (File f : fileList) {
            // 生成.s文件
            //如果时间超过了10s，那么就会抛出异常

            try{
                Compiler.main(new String[]{"-S", "-o", "./tests/test1.s", f.getAbsolutePath()});
            } catch (Exception e) {
                System.err.println("Failed to compile " + f.getName());
                continue;
            }
            //统计.s文件的行数
            FileReader reader = new FileReader("./tests/test1.s");
            int lines = 0;
            int c;
            while ((c = reader.read()) != -1) {
                if (c == '\n') {
                    lines++;
                }
            }
            //输出.s文件的行数
            System.out.println(f.getName() + " has " + lines + " lines");

            //riscv64-linux-gnu-gcc -c ./tests/test1.s -o ./tests/test1.o -static
            ProcessBuilder pb1 = new ProcessBuilder("riscv64-linux-gnu-gcc", "-c", "./tests/test1.s", "-o", "./tests/test1.o", "-static");
            Process process = pb1.start();
            if(!process.waitFor(timeOut, java.util.concurrent.TimeUnit.SECONDS)){
                //如果超过10s，就杀死进程
                process.destroy();
                System.err.println("Time out while compiling .s file for " + f.getName());
                continue;
            }
            int processExitCode = process.waitFor();
            if(processExitCode != 0) {
                System.err.println("riscv64-linux-gnu-gcc failed for " + f.getName());
                continue;
            }


            //riscv64-linux-gnu-gcc ./tests/test1.o ./res/riscv_libsylib.a -o ./tests/test1 -static
            ProcessBuilder pb2 = new ProcessBuilder("riscv64-linux-gnu-gcc", "./tests/test1.o", "./riscv_libsylib.a", "-o", "./tests/test1", "-static");
            process = pb2.start();
            if(!process.waitFor(timeOut, java.util.concurrent.TimeUnit.SECONDS)){
                //如果超过10s，就杀死进程
                process.destroy();
                System.err.println("Time out while linking lib.a file for " + f.getName());
                continue;
            }
            processExitCode = process.waitFor();
            if(processExitCode != 0) {
                System.err.println("riscv64-linux-gnu-gcc failed for " + f.getName()+"while linking lib.a file");
                continue;
            }

            // 查找对应的输入文件
            String name = f.getName();
            String inName = name.substring(0, name.length() - 3) + ".in";
            File inFile = new File(testPath + "/" + inName);

            // 程序的return值存在processExitCode中，PRINT的输出存在output文件中
            // 使用生成的可执行文件进行测试
            //qemu-riscv64 ./tests/test1 < ./tests/test1.in > ./tests/test1.out; echo $$?
            ProcessBuilder pb3;
            if (inFile.exists()) {
                pb3 = new ProcessBuilder("qemu-riscv64", "./tests/test1");
                pb3.redirectInput(inFile);
            } else {
                pb3 = new ProcessBuilder("qemu-riscv64", "./tests/test1");
            }
            File outputFile = new File("./tests/test1.out");
            pb3.redirectOutput(outputFile);//print的输出被重定向到output文件中
            process = pb3.start();
            //如果等待时间超过了指定的时间，那么就会抛出异常
            if(!process.waitFor(timeOut, java.util.concurrent.TimeUnit.SECONDS)){
                //如果超过10s，就杀死进程
                process.destroy();
                System.err.println("Time out while running test for " + f.getName());
                continue;
            }
            processExitCode = process.waitFor();//程序的return值存在processExitCode中
            // 读取测试输出PRINT的文件,可能为空
            FileReader inReader = new FileReader(outputFile);
            StringBuilder inBuilder = new StringBuilder();
            while ((c = inReader.read()) != -1) {
                inBuilder.append((char) c);
            }
            inReader.close();


            // 查找对应的.out文件
            String outName = name.substring(0, name.length() - 3) + ".out";
            File outFile = new File(testPath + "/" + outName);
            if (!outFile.exists()) {
                System.out.println("Reference output file not found: " + outName);
                continue;
            }
            // 读取参考输出文件
            FileReader outReader = new FileReader(outFile);
            StringBuilder outBuilder = new StringBuilder();
            while ((c = outReader.read()) != -1) {
                outBuilder.append((char) c);
            }
            outReader.close();

            //outreader的最后一行是return值，前面是print的输出
            String out = outBuilder.toString();
            String in = inBuilder.toString();
            //截取out的最后一行
            int index = out.lastIndexOf("\n") - 1;
            while(out.charAt(index) != '\n'&& index != 0) {
                index--;
            }
            if(index == 0){
                index = -1;
            }
            String outReturn = out.substring(index + 1, out.length()- 1);
            out = out.substring(0,index + 1);
            int return_value = Integer.parseInt(outReturn);

            in = in.trim();
            out = out.trim();
            if(processExitCode == return_value && in.equals(out)) {
                System.out.println(f.getName() + " passed");
            } else {
                System.err.println(f.getName() + " failed for " + f.getName() + " with return value " + processExitCode + " and output " + in);
            }
        }
    }
}
