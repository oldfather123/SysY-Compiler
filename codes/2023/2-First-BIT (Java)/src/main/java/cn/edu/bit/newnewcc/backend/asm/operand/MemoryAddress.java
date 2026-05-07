package cn.edu.bit.newnewcc.backend.asm.operand;

public class MemoryAddress extends AsmOperand implements RegisterReplaceable {
    private final long offset;
    private final IntRegister baseAddress;

    public MemoryAddress(long offset, IntRegister baseAddress) {
        this.offset = offset;
        this.baseAddress = baseAddress;
    }

    public MemoryAddress withBaseRegister(IntRegister newBaseRegister) {
        return new MemoryAddress(getOffset(), newBaseRegister);
    }

    public MemoryAddress addOffset(long diff) {
        return new MemoryAddress(getOffset() + diff, getBaseAddress());
    }

    public MemoryAddress setOffset(long newOffset) {
        return new MemoryAddress(newOffset, getBaseAddress());
    }

    public MemoryAddress getAddress() {
        return new MemoryAddress(getOffset(), getBaseAddress());
    }

    public long getOffset() {
        return offset;
    }

    public IntRegister getBaseAddress() {
        return baseAddress;
    }

    @Override
    public IntRegister getRegister() {
        return getBaseAddress();
    }

    @Override
    public MemoryAddress withRegister(Register register) {
        if (!(register instanceof IntRegister))
            throw new IllegalArgumentException();

        return withBaseRegister((IntRegister) register);
    }

    public String emit() {
        return String.format("%d(%s)", getOffset(), getBaseAddress().emit());
    }

    @Override
    public String toString() {
        return String.format("MemoryAddress(%s, %s)", getOffset(), getBaseAddress());
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        MemoryAddress address = (MemoryAddress) o;

        if (offset != address.offset) return false;
        return baseAddress.equals(address.baseAddress);
    }

    @Override
    public int hashCode() {
        int result = (int) (offset ^ (offset >>> 32));
        result = 31 * result + baseAddress.hashCode();
        return result;
    }
}
