/*********************************************************************************************************************
 * COPYRIGHT NOTICE
 * Copyright (c) 2020,逐飞科技
 * All rights reserved.
 * 技术讨论QQ群：一群：179029047(已满)  二群：244861897(已满)  三群：824575535
 *
 * 以下所有内容版权均属逐飞科技所有，未经允许不得用于商业用途，
 * 欢迎各位使用并传播本程序，修改内容时必须保留逐飞科技的版权声明。
 *
 * @file       		main
 * @company	   		成都逐飞科技有限公司
 * @author     		逐飞科技(QQ790875685)
 * @version    		查看doc内version文件 版本说明
 * @Software 		MDK FOR C251 V5.60
 * @Target core		STC32G12K128
 * @Taobao   		https://seekfree.taobao.com/
 * @date       		2020-12-18
 ********************************************************************************************************************/

#include "headfile.h"

/*
 * 系统频率，可查看board.h中的 FOSC 宏定义修改。
 * board.h文件中FOSC的值设置为0,则程序自动设置系统频率为33.1776MHZ
 * 在board_init中,已经将P54引脚设置为复位
 * 如果需要使用P54引脚,可以在board.c文件中的board_init()函数中删除SET_P54_RESRT即可
 */
 
// *************************** 例程使用步骤说明 ***************************
// 1.根据硬件连接说明连接好模块，使用电源供电(下载器供电会导致模块电压不足)
//
// 2.下载例程到单片机中，打开逐飞串口助手。
//
// 3.在逐飞串口助手中，选择示波器。
//
// 4.选择下载器对应的串口号，波特率(默认115200)，点击连接
//
// 5.示波器能立刻看到波形

// *************************** 例程测试说明 ***************************
// 1.本例程会通过 Debug 串口输出测试信息 请务必接好调试串口以便获取测试信息
//
// 2.连接好模块和核心板后（尽量使用配套主板测试以避免供电不足的问题） 烧录本例程 按下复位后程序开始运行
//


//-------------------------------------------------------------------------------------------------------------------
// 函数简介     滴答客发送函数
// 参数说明     *buff           需要发送的数据地址
// 参数说明     length          需要发送的长度
// 返回参数     uint32          剩余未发送数据长度
// 使用示例
//-------------------------------------------------------------------------------------------------------------------
uint32 seekfree_assistant_transfer_callback   (const uint8 *buff, uint32 length)
{
	uart_putbuff(DEBUG_UART, buff, length);
	return 0;
}

void main()
{
	board_init();			// 初始化寄存器,勿删除此句代码。
	
	// 设置函数指针
	seekfree_assistant_transfer = seekfree_assistant_transfer_callback;

	
	// 此处编写用户代码(例如：外设初始化代码等)
	
	// 设置数据
	seekfree_assistant_oscilloscope_data.dat[0] = 1;
	seekfree_assistant_oscilloscope_data.dat[1] = 2;
	seekfree_assistant_oscilloscope_data.dat[2] = 3.00;
	seekfree_assistant_oscilloscope_data.dat[3] = 3.22;
	// 需要传输四个通道数据
	seekfree_assistant_oscilloscope_data.channel_num = 4;
	
    while(1)
	{
		seekfree_assistant_oscilloscope_data.dat[0]++;
		seekfree_assistant_oscilloscope_data.dat[1]++;
		seekfree_assistant_oscilloscope_data.dat[2] += 3;
		seekfree_assistant_oscilloscope_data.dat[3] += 4;
		
		// 通过串口发送虚拟示波器数据
		seekfree_assistant_oscilloscope_send(&seekfree_assistant_oscilloscope_data);
		
		delay_ms(1);
		// 此处编写需要循环执行的代码
    }
}




