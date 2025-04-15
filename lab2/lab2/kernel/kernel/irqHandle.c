#include "x86.h"
#include "device.h"

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

int tail=0;

void GProtectFaultHandle(struct TrapFrame *tf);

void KeyboardHandle(struct TrapFrame *tf);

void timerHandler(struct TrapFrame *tf);
void syscallHandle(struct TrapFrame *tf);
void sysWrite(struct TrapFrame *tf);
void sysPrint(struct TrapFrame *tf);
void sysRead(struct TrapFrame *tf);
void sysGetChar(struct TrapFrame *tf);
void sysGetStr(struct TrapFrame *tf);
void sysSetTimeFlag(struct TrapFrame *tf);
void sysGetTimeFlag(struct TrapFrame *tf);


void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));

	switch(tf->irq) {
		// TODO: 填好中断处理程序的调用
		case 0xd:
		GProtectFaultHandle(tf);
			break;
		case 0x21:
			KeyboardHandle(tf);
			break;
		case 0x80:
			syscallHandle(tf);
			break;
		default:assert(0);
	}
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}

void KeyboardHandle(struct TrapFrame *tf){
	uint32_t code = getKeyCode();

	if(code == 0xe){ // 退格符
		//要求只能退格用户键盘输入的字符串，且最多退到当行行首
		if(displayCol>0&&displayCol>tail){
			displayCol--;
			uint16_t data = 0 | (0x0c << 8);
			int pos = (80*displayRow+displayCol)*2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
		}
	}else if(code == 0x1c){ // 回车符
		//处理回车情况
		keyBuffer[bufferTail++]='\n';
		displayRow++;
		displayCol=0;
		tail=0;
		if(displayRow==25){
			scrollScreen();
			displayRow=24;
			displayCol=0;
		}
	}else if(code < 0x81){ 
		// TODO: 处理正常的字符

		// char ch = getChar(code);
		// if (ch >= 0x20) {
		// 	putChar(ch);
		// 	keyBuffer[bufferTail++] = ch;
		// 	// 维护光标位置
		// 	int segment_selector = USEL(SEG_UDATA);
		// 	asm volatile("movw %0, %%es"::"m"(segment_selector));

		// 	// 打印字符到显存
		// 	// 0xb8000是显存的起始地址
		// 	// 每个字符占2个字节，第一个字节是字符，第二个字节是属性
		// 	// 0x0c是白底黑字
		// 	// 0x0f是黑底白字
		// 	uint16_t data = ch | (0x0c << 8);
		// 	int pos = (80 * displayRow + displayCol) * 2;
		// 	asm volatile("movw %0, (%1)"::"r"(data), "r"(pos + 0xb8000));

		// 	// 更新光标位置
		// 	displayCol ++;
		// 	if (displayCol >= 80) {
		// 		displayCol = 0;
		// 		displayRow ++;
		// 	}
		// 	while (displayRow >= 25) {
		// 		scrollScreen();
		// 		displayRow --;
		// 		displayCol = 0;
		// 	}	
		// 	if(bufferTail >= MAX_KEYBUFFER_SIZE) {
		// 		assert(0);
		// 	}
		// }
		char ch = getChar(code);
		if (ch >= 0x20) {
			putChar(ch);
			keyBuffer[bufferTail++] = ch;
			int sel = USEL(SEG_UDATA);
			asm volatile("movw %0, %%es"::"m"(sel));

			uint16_t data = ch | (0x0c << 8);
			int pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)"::"r"(data), "r"(pos + 0xb8000));
				
			displayCol ++;
			if (displayCol >= 80) {
				displayCol = 0;
				displayRow ++;
			}
			while (displayRow >= 25) {
				scrollScreen();
				displayRow --;
				displayCol = 0;
			}	
		}
	}
	updateCursor(displayRow, displayCol);
}


	// 定义一个全局计数器
volatile int timerCounter = 0;

void timerHandler(struct TrapFrame *tf) {
		// TODO: 处理定时器中断

    // 每次定时器中断，计数器加一
    timerCounter++;

    // 每 100 次定时器中断打印一条信息
    if (timerCounter % 100 == 0) {
        // 这里可以调用一个函数来打印信息到屏幕
        // 例如，假设我们有一个 putChar 函数可以打印字符
        char message[] = "Timer interrupt occurred 100 times!\n";
        for (int i = 0; message[i] != '\0'; i++) {
            putChar(message[i]);
        }
    }

    // 向 8259A 中断控制器发送 EOI（End of Interrupt）信号
    // 0x20 是主 8259A 中断控制器的端口地址
    outByte(0x20, 0x20);
}



void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case 0:
			sysWrite(tf);
			break; // for SYS_WRITE
		case 1:
			sysRead(tf);
			break; // for SYS_READ
		default:break;
	}
}

void sysWrite(struct TrapFrame *tf) {
	switch(tf->ecx) { // file descriptor
		case 0:
			sysPrint(tf);
			break; // for STD_OUT
		default:break;
	}
}

void sysPrint(struct TrapFrame *tf) {
	int sel =  USEL(SEG_UDATA);
	char *str = (char*)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str+i));
		// TODO: 完成光标的维护和打印到显存
		if (character == '\n') {
			displayRow += 1;
			displayCol = 0;
		}
		else {
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)"::"r"(data), "r"(pos + 0xb8000));
			displayCol ++;
		}

		if (displayCol >= 80) {
			displayCol = 0;
			displayRow ++;
		}
		while (displayRow >= 25) {
			scrollScreen();
			displayRow --;
			displayCol = 0;
		} 
	}
	tail=displayCol;
	updateCursor(displayRow, displayCol);
}

void sysRead(struct TrapFrame *tf){
	switch(tf->ecx){ //file descriptor
		case 0:
			sysGetChar(tf);
			break; // for STD_IN
		case 1:
			sysGetStr(tf);
			break; // for STD_STR
		default:break;
	}
}

void sysGetChar(struct TrapFrame *tf){
	// TODO: 自由实现
	int flag = 0;
	if(keyBuffer[bufferTail-1] == '\n') flag = 1;
	while(bufferTail > bufferHead && keyBuffer[bufferTail-1] == '\n') 
		keyBuffer[--bufferTail] = '\0';
	if(bufferTail > bufferHead && flag){ 
		tf->eax = keyBuffer[bufferHead++];
		bufferHead = bufferTail;
	}
	else {
		tf->eax = 0;
	}
}

void sysGetStr(struct TrapFrame *tf){
	// TODO: 自由实现
	int size = tf->ebx;
	char *str = (char*)tf->edx;
	int i = 0;
	
	int sel = USEL(SEG_UDATA);
	asm volatile("movw %0, %%es"::"m"(sel));

	int flag = 0;
	if(keyBuffer[bufferTail-1] == '\n') flag = 1;
	while(bufferTail > bufferHead && keyBuffer[bufferTail-1] == '\n') 
		keyBuffer[--bufferTail] = '\0';
	if(!flag && bufferTail-bufferHead < size) {
		tf->eax = 0;
	}
	else{
		for(i=0; i<size && i<bufferTail-bufferHead;i++){
			char ch = keyBuffer[bufferHead+i];
			asm volatile("movb %0, %%es:(%1)"::"r"(ch),"r"(str+i));
		}
		tf->eax = 1;
	}
}

void sysGetTimeFlag(struct TrapFrame *tf) {
	// TODO: 自由实现

}

void sysSetTimeFlag(struct TrapFrame *tf) {
	// TODO: 自由实现

}

// BCD 转换为十进制
static inline int bcdToDec(uint8_t bcd) {
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

