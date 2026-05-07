package Utils;

import Backend.Backend;
import Backend.component.ObjBlock;
import Backend.component.ObjFunction;
import Backend.component.ObjGlobalVariable;
import Backend.component.ObjModule;
import Backend.instruction.ObjInstr;
import Driver.Config;
import Utils.DataStruct.IList;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;

public class RISC_Dump {
	private static BufferedWriter out;
//    static {
//        try {
//            out = new BufferedWriter(new FileWriter(Config.outputFile));
//        } catch (IOException e) {
//            throw new RuntimeException(e);
//        }
//    }

	public static void DumpObjBlocks(ObjBlock objBlock) throws IOException {
		out.write(objBlock.getName() + ":\n");
		// System.out.println(objBlock.getName());
		for (IList.INode<ObjInstr, ObjBlock> instr : objBlock.getInstrs()) {
			ObjInstr i = instr.getValue();
			// System.out.println("\t" + i.toString());
			out.write("\t" + i.toString() + "\n");
		}
	}

	public static void DumpObjFunction(ObjFunction objFunction) throws IOException {
		if (objFunction.getName().equals("main")) {
			out.write(".section\t.text.startup\n");
			out.write(".align\t1\n");
			out.write(".globl\tmain\n");
		} else {
			out.write(".section\t.text\n");
			out.write(".align\t1\n");
		}
		out.write(objFunction.getName() + ":\n");
		// System.out.println(objFunction.getName());
		IList<ObjBlock, ObjFunction> bs = objFunction.getObjBlocks();
		for (IList.INode<ObjBlock, ObjFunction> b : bs)
			DumpObjBlocks(b.getValue());
	}


	public static void DumpObjModle(ObjModule objModule) throws IOException {

		ArrayList<ObjGlobalVariable> globalVariables = objModule.getGlobalVariables();
		for (ObjGlobalVariable g : globalVariables)
			out.write(g.toString() + "\n");

		ArrayList<ObjFunction> functions = objModule.getFunctions();
		for (ObjFunction f : functions)
			DumpObjFunction(f);

	}

	public static void DumpBackend(Backend backend) throws IOException {
		DumpBackend(backend, Config.outputFile);
	}

	public static void DumpBackend(Backend backend, String filename) throws IOException {
		out = new BufferedWriter(new FileWriter(filename));
		DumpObjModle(backend.getModule());
		out.close();
	}
	public static void DumpObjModle(ObjModule objModule, String filename) throws IOException {
		out = new BufferedWriter(new FileWriter(filename));
		DumpObjModle(objModule);
		out.close();
	}
}
