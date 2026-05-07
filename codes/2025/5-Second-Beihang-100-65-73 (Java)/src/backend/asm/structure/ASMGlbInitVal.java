package backend.asm.structure;

public abstract class ASMGlbInitVal {
    public static class ASMGlbWord extends ASMGlbInitVal {
        private final int value;
        
        public ASMGlbWord(int value) {
            this.value = value;
        }
        
        @Override
        public String toString() {
            return ".word " + this.value;
        }
    }
    public static class ASMGlbZero extends ASMGlbInitVal {
        private int number;
        
        public ASMGlbZero(int number) {
            this.number = number;
        }
        
        public int getNumber() {
            return number;
        }
        
        /**
         * 吃掉另一个（与之合并）
         * @param ano 被吃掉的
         */
        public void merge(ASMGlbZero ano) {
            this.number += ano.number;
        }
        
        @Override
        public String toString() {
            return ".zero " + number * 4;
        }
    }
}
