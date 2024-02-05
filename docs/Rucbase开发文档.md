# Rucbase开发文档

## flex && bison文件的修改
在parser子文件夹下涉及flex和bison文件的修改，开发者修改lex.l和yacc.y文件之后，需要通过以下命令重新生成对应文件：
```bash
flex --header-file=lex.yy.hpp -o lex.yy.cpp lex.l
bison --defines=yacc.tab.hpp -o yacc.tab.cpp yacc.y
```

## 代码规范
> 以VScode format配置为例

```
{ 
    BasedOnStyle: Google, 
    IndentWidth: 4,
    TabWidth: 4, 
    ColumnLimit: 120
}
```

## 注释规范
使用`/** */`写注释块

使用`//` 写行内注释

使用`//*/`作为代码注释开关

尽量不要使用中文标点符号
注释块符号
```
@brief: 简要描述
@param: 参数描述
@return: 用它来制定一个 method 或 function的返回值
@note: 注意点
@see: 用它来指明其他相关的 method 或 function . 你可以使用多个这种标签.
```
### example
```cpp

/**
 * @brief: 初始化系统.
 * @note: 动态参数不可为0
 * @param: dynamParam 动态参数
 * @param: controlPrarm 控制参数
 * @return: true初始化成功, false初始化失败
*/
bool init(int dynamParam,int controlPrarm){
    //*/
        codeSegement1;  
    /*/
        codeSegement2;  
    //*/
}

```

## 相关资料
[GoogleTest框架简介](https://www.cnblogs.com/jycboy/p/6057677.html)