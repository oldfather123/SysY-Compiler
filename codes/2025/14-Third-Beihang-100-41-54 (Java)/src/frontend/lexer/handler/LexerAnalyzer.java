package frontend.lexer.handler;

import frontend.lexer.entity.Token;
import frontend.lexer.entity.TokenList;
import frontend.lexer.entity.TokenType;

import java.io.*;
import java.util.Scanner;

public class LexerAnalyzer {

    public TokenList tokenList = new TokenList();
    private BufferedReader bufferedReader;
    private BufferedWriter bufferedWriter;
    //以下变量用来表示正在构建的单词
    private StringBuilder tokenLine = new StringBuilder();

    public LexerAnalyzer(BufferedReader bufferedReader, BufferedWriter bufferedWriter){
        this.bufferedReader = bufferedReader;
        this.bufferedWriter = bufferedWriter;
    }

    public char read(){
        try{
            int characterRead = bufferedReader.read();
            if(characterRead < 0) return 0;
            else return (char)characterRead;
        }catch (Exception e){
            return 0;
        }
    }

    public boolean isBlank(char c){
        return Character.isWhitespace(c);
    }


    //该方法用于跳过//这样的注释
    public void nextline(){
        char c;
        do {
            c = read();
        } while (c != 0 && c != '\n');
        Token.addLineNumber();
        tokenLine = new StringBuilder();
    }

    //该方法用于跳过/*这样的注释
    public void nextpart(){
        char c;char last = '\0';
        while(true){
            c = read();
            if(c=='\n') Token.addLineNumber();
            if(c == '/' && last == '*') break;
            last = c;
        }
        tokenLine = new StringBuilder();
    }

    //该方法用于增加一个新token
    public void addToken(){
        String str = new String(tokenLine);
        this.tokenList.addNewToken(new Token(TokenType.parseWordType(str),str));
        tokenLine = new StringBuilder();
    }

    //该方法用于错误处理，暂未开发
    public void addError(){
        System.err.println("Wrong Input!!");
    }
    public void WordAnalyze(){
        boolean hasRead = false;char nowReadChar = '\0';
        while(true){
            if(!hasRead) nowReadChar = read();
            else hasRead = false;

            if(nowReadChar == 0) {
                if(!tokenLine.isEmpty()) addToken();
                break;
            }
            if(nowReadChar == '\n') Token.addLineNumber();

            if(isBlank(nowReadChar)){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine = new StringBuilder();
            }
            //处理/,//,/*的情况
            else if(nowReadChar == '/'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);
                nowReadChar = read();
                if(nowReadChar == '/') nextline();
                else if(nowReadChar == '*') nextpart();
                else{
                    hasRead = true;
                    addToken();
                }
            }
            //处理字符串常量
            else if(nowReadChar == '\"'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);
                while((nowReadChar = read())!= 0){
                    tokenLine.append(nowReadChar);
                    if(nowReadChar == '\n') Token.addLineNumber();
                    if(nowReadChar == '\"') break;
                }
                addToken();
            }
            //处理字符常量
            else if(nowReadChar == '\''){
                //记录第一个'
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);
                nowReadChar = read();
                tokenLine.append(nowReadChar);
                if(nowReadChar == '\\'){
                    nowReadChar = read();
                    tokenLine.append(nowReadChar);
                }
                nowReadChar = read();
                tokenLine.append(nowReadChar);
                addToken();
            }
            //处理! !=的情况
            else if(nowReadChar == '!'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);
                nowReadChar = read();
                if(nowReadChar == '='){
                    tokenLine.append(nowReadChar);
                    addToken();
                }
                else{
                    hasRead = true;
                    addToken();
                }
            }
            //处理&&的情况
            else if(nowReadChar == '&'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);
                nowReadChar = read();
                if(nowReadChar == '&'){
                    tokenLine.append(nowReadChar);addToken();
                }
                else{
                    hasRead = true;addError();
                }
            }
            //处理||的情况
            else if(nowReadChar == '|'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);
                nowReadChar = read();
                if(nowReadChar == '|'){
                    tokenLine.append(nowReadChar);addToken();
                }
                else{
                    hasRead = true;addError();
                }
            }
            else if(nowReadChar == '+'){
                if(!tokenLine.isEmpty()){
                    if(tokenLine.toString().matches("[0-9]+[eEpP]")
                            || tokenLine.toString().matches("[0-9]*+\\.[0-9]*[eEpP]") ||
                            tokenLine.toString().matches("0[xX]([0-9A-Fa-f]+\\.?[0-9A-Fa-f]*|\\.[0-9A-Fa-f]+)[pP]")){
                        tokenLine.append(nowReadChar); continue;
                    }
                    else addToken();
                }
                tokenLine.append(nowReadChar);addToken();
            }
            else if(nowReadChar == '-'){
                if(!tokenLine.isEmpty()){
                    if(tokenLine.toString().matches("[0-9]+[eEpP]")
                            || tokenLine.toString().matches("[0-9]*+\\.[0-9]*[eEpP]") ||
                            tokenLine.toString().matches("0[xX]([0-9A-Fa-f]+\\.?[0-9A-Fa-f]*|\\.[0-9A-Fa-f]+)[pP]")){
                        tokenLine.append(nowReadChar); continue;
                    }
                    else addToken();
                }
                tokenLine.append(nowReadChar);addToken();
            }
            else if(nowReadChar == '*'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);addToken();
            }
            else if(nowReadChar == '%'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);addToken();
            }
            //处理< <=的情况
            else if(nowReadChar == '<'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);
                nowReadChar = read();
                if(nowReadChar == '='){
                    tokenLine.append(nowReadChar);addToken();
                }
                else{
                    hasRead = true;addToken();
                }
            }
            //处理> >=的情况
            else if(nowReadChar == '>'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);
                nowReadChar = read();
                if(nowReadChar == '='){
                    tokenLine.append(nowReadChar);addToken();
                }
                else{
                    hasRead = true;addToken();
                }
            }
            //处理= ==的情况
            else if(nowReadChar == '='){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);
                nowReadChar = read();
                if(nowReadChar == '='){
                    tokenLine.append(nowReadChar);addToken();
                }
                else{
                    hasRead = true;addToken();
                }
            }
            else if(nowReadChar == ';'){
                if(!tokenLine.isEmpty()) addToken();
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);addToken();
            }
            else if(nowReadChar == ','){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);addToken();
            }
            else if(nowReadChar == '('){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);addToken();
            }
            else if(nowReadChar == ')'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);addToken();
            }
            else if(nowReadChar == '['){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);addToken();
            }
            else if(nowReadChar == ']'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);addToken();
            }
            else if(nowReadChar == '{'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);addToken();
            }
            else if(nowReadChar == '}'){
                if(!tokenLine.isEmpty()) addToken();
                tokenLine.append(nowReadChar);addToken();
            }
            else tokenLine.append(nowReadChar);
        }
    }

    public void WordListPrintText() throws IOException {
        for(Token token:tokenList.tokenList){
            bufferedWriter.write(token.line + ":  " + token.wordType + "   "  + token.content  + "\n");
        }
    }
}