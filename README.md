# 实验一：带键盘事件的计算器

Qt6 Widgets 实现的四则运算计算器，界面用 Qt Designer 布局，鼠标与键盘共用同一套输入处理逻辑。

## 1. 开发环境

| 项目 | 版本 |
| --- | --- |
| 操作系统 | Windows 11 64 位 |
| Qt | Qt 6.10.3（MinGW 13.1.0 64-bit 套件） |
| 构建工具 | CMake 3.30 + Ninja |
| 编辑器 | Qt Creator |

> 实验指导书要求 Qt 6.9.2，本机安装的是 Qt 6.10.3，工程只用到 QtWidgets 的基础能力，两者行为一致。

## 2. 构建与运行

### 用 Qt Creator

1. 打开 `CMakeLists.txt`，选择 `Desktop Qt 6.10.3 MinGW 64-bit` 套件；
2. 直接构建运行（Qt Creator 会自己生成 shadow build 目录）。

### 用命令行

```bat
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="D:/Qt/6.10.3/mingw_64"
cmake --build build
build\Exp1_KeyboardCalculator.exe
```

> 注意：**构建目录不要放在含中文的路径下**。本工程源码位于 `D:\Cici\QQ\Qt课程作业\`，
> 如果把构建目录也放在这里，moc 在 GBK 代码页下会报
> `Cannot create .../moc_calculatorwindow.cpp: No such file or directory`。
> 把 `-B` 指向纯英文路径（如 `-B C:/temp/qcalc-build`）即可正常构建。

## 3. 目录结构

```
Exp1_KeyboardCalculator/
├── CMakeLists.txt          # 工程配置，开启 AUTOUIC/AUTOMOC/AUTORCC
├── main.cpp                # 入口，加载 qrc 中的样式表
├── calculatorwindow.ui     # Qt Designer 界面：显示区 + 5×4 按键网格
├── calculatorwindow.h      # 主窗口类声明
├── calculatorwindow.cpp    # 计算逻辑、按键处理、键盘事件
├── resources.qrc           # 资源文件
├── style.qss               # QSS 样式表
└── README.md
```

## 4. 界面与主要控件

| 控件 | 类型 | 作用 |
| --- | --- | --- |
| `displayFrame` | QFrame | 显示区容器，白底圆角卡片 |
| `lblExpression` | QLabel | 表达式行（灰色小字），显示"操作数 运算符"与等式 |
| `lblDisplay` | QLabel | 主显示区（40px 深色粗体），显示当前输入或结果 |
| `buttonGrid` | QGridLayout | 5 行 4 列按键网格，间距 6px |
| `btn0`~`btn9` / `btnDot` | QPushButton | 数字与小数点 |
| `btnAdd` `btnSubtract` `btnMultiply` `btnDivide` | QPushButton | 四则运算符 |
| `btnEqual` `btnClear` `btnBackspace` `btnSign` `btnPercent` | QPushButton | 等号、清除、退格、正负号、百分号 |

布局方式：整体 `QVBoxLayout`（显示区在上、按键网格在下），网格用 `QGridLayout`，
每个按钮 `minimumSize` 60×54 且四边 `Expanding`，窗口固定 330×480，缩放时按钮自动均分。
每个按钮额外设置了两个动态属性：`calcKey`（按键语义）与 `role`（样式类别，供 QSS 选择器使用）。

## 5. 状态与逻辑

计算器只有四个状态变量：

| 成员 | 含义 |
| --- | --- |
| `m_operand` | 已锁定的第一个操作数（字符串保存，便于原样显示） |
| `m_pendingOp` | 待执行的运算符，空表示没有待运算表达式 |
| `m_isNewInput` | 下一次按数字是否覆盖显示区（按完运算符/等号后为 `true`） |
| `m_errorState` | 错误态（除数为零），此时只接受"清除"或直接输入新数字 |

鼠标与键盘两条路径在 `handleKey()` 汇合：

```
鼠标：QPushButton::clicked -> onButtonClicked() -> calcKey -> handleKey()
键盘：keyPressEvent() -> keyToCalcKey() ---------------------> handleKey()
```

## 6. 按键映射

| 键盘 | 等价按钮 |
| --- | --- |
| `0`~`9`（含小键盘） | 数字键 |
| `.` / `,` | 小数点 |
| `+` `-` `*` `x` `/` | 四则运算符 |
| `Enter` / `=` | 等号 |
| `Backspace` | 退格 |
| `Esc` / `Delete` / `C` | 清除 |
| `%` | 百分号 |

## 7. 已处理的边界情况

| 输入 | 处理结果 |
| --- | --- |
| `1..2` | 第二个小数点被忽略，得到 `1.2` |
| `5 + × 3 =` | 只替换运算符，得到 `15`（不是 `30`） |
| `5 ÷ 0 =` | 显示"除数不能为零"，不再产生 `inf` 或崩溃 |
| `5 + 3 - 2 =` | 按 `-` 时先结算 `5+3=8`，最终 `6` |
| `2 + 3 = 4` | 结果后直接按数字，开始新的一次输入，显示 `4` |
| `50 + 3 = + 7 =` | 结果继续参与下一次运算，得到 `60` |
| `123456` 后连按两次退格 | 得到 `1234`，退到空时回落到 `0` |
| 除零后按数字 | 自动清零并开始新输入 |

## 8. 已知不足

- 没有实现历史记录、括号与优先级，连续运算按从左到右顺序结算；
- 只保留 double 精度，`0.1+0.2` 这类结果会显示为 `0.3`（格式化时截断到 10 位小数），未做十进制精确运算；
- 百分号语义为简化实现：加减法取第一个操作数的百分比，乘除法直接除以 100。
