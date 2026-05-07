package backend.asm.register.store;

import backend.asm.register.phy.PhyIReg;
import backend.asm.register.phy.PhyReg;

import java.util.*;

public abstract class RegStore {
    public static final PhyReg NA = new PhyIReg("NA", "NA"); // Not Available, used for instructions that do not require a register

    protected final PhyReg zero;
    protected final PhyReg ra;
    protected final PhyReg sp;
    protected final List<PhyReg> intTempRegs = new ArrayList<>();
    protected final List<PhyReg> intSavedRegs = new ArrayList<>();
    protected final List<PhyReg> intArgRegs = new ArrayList<>();
    protected final List<PhyReg> fltTempRegs = new ArrayList<>();
    protected final List<PhyReg> fltSavedRegs = new ArrayList<>();
    protected final List<PhyReg> fltArgRegs = new ArrayList<>();
    protected final Set<PhyReg>  allTempRegs = new LinkedHashSet<>();

    protected RegStore(PhyReg zero, PhyReg ra, PhyReg sp) {
        this.zero = zero;
        this.ra = ra;
        this.sp = sp;
    }

    //TODO: 确认临时寄存器与保存寄存器的使用
    public List<PhyReg> getAllIntTempRegs() {
        return intTempRegs;
    }

    public List<PhyReg> getAllIntSavedRegs() {
        return intSavedRegs;
    }

    public PhyReg getIntArgRegByNumber(int number) {
        return intArgRegs.get(number);
    }

    public PhyReg getIntSavedRegByNumber(int number) {
        return intSavedRegs.get(number);
    }

    //TODO: 确认临时寄存器与保存寄存器的使用
    public List<PhyReg> getAllFltTempRegs() {
        return fltTempRegs;
    }

    public List<PhyReg> getAllFltSavedRegs() {
        return fltSavedRegs;
    }

    public PhyReg getFltArgRegByNumber(int number) {
        return fltArgRegs.get(number);
    }

    public PhyReg getFltSavedRegByNumber(int number) {
        return fltSavedRegs.get(number);
    }

    public PhyReg getZero() {
        return zero;
    }

    public PhyReg getStackPtr() {
        return sp;
    }

    public PhyReg getRetAddr() {
        return ra;
    }

    public PhyReg getIntRetVal() {
        return intArgRegs.getFirst();
    }

    public PhyReg getFltRetVal() {
        return fltArgRegs.getFirst();
    }

    public abstract PhyIReg getReservedReg();

    public Set<PhyReg> getAllTempRegs() {
        return allTempRegs;
    }

    public List<PhyReg> getIntArgRegs() {
        return intArgRegs;
    }

    public List<PhyReg> getFltArgRegs() {
        return fltArgRegs;
    }
}
