package cn.edu.bit.newnewcc.ir.util;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.pass.ir.IrNamingPass;

import java.io.FileWriter;
import java.io.IOException;

public class IrEmitter {

    public static void emitToFile(Module module, String fileName) {
        IrNamingPass.runOnModule(module);
        var builder = new StringBuilder();
        module.emitIr(builder);
        try (var fileWriter = new FileWriter(fileName)) {
            fileWriter.write(builder.toString());
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static void emitToConsole(Module module) {
        IrNamingPass.runOnModule(module);
        var builder = new StringBuilder();
        module.emitIr(builder);
        System.out.println(builder);
    }

}
