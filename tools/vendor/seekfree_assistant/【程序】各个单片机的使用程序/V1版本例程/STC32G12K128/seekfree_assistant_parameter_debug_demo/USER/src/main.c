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
// 3.在逐飞串口助手中，选择示波器。点击开启调参。
//
// 4.选择下载器对应的串口号，波特率(默认115200)，点击连接
//
// 5.拖动参数进度条或者点击+或者-就能看到串口助手的数据发生改变。

// *************************** 例程测试说明 ***************************
// 1.本例程会通过 Debug 串口输出测试信息 请务必接好调试串口以便获取测试信息
//
// 2.连接好模块和核心板后（尽量使用配套主板测试以避免供电不足的问题） 烧录本例程 按下复位后程序开始运行
//
// 3.例程中使用FIFO的原因：
//
//	由于串口接收在中断里面，为了方便处理数据，且不影响串口中断
//	所以，这里使用了FIFO进行处理。
// 	进入串口中断，将数据存入FIFO中。
// 	在主循环里面对FIFO的数据进行读取并处理。
// 	如果在中断里面进行数据读取，并处理。将会导致下一个串口数据已经接收到了，
// 	现在还没有处理完数据,这样串口数据就会丢失。

#define TEMP_BUFFER_SIZE  	64
static  fifo_struct     	temp_uart_fifo;
static  uint8           	temp_uart_buffer[TEMP_BUFFER_SIZE];  // 数据存放数组

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     滴答客接收数据函数
// 参数说明     *buff           需要接收的数据地址
// 参数说明     length          要接收的数据最大长度
// 返回参数     uint32          接收到的数据长度
// 使用示例
//-------------------------------------------------------------------------------------------------------------------
uint32 seekfree_assistant_receive_callback   (const uint8 *buff, uint32 length)
{
	fifo_read_buffer(&temp_uart_fifo, buff, &length, FIFO_READ_AND_CLEAN);
	return length;
}

uint8 i = 0;

void main()
{
	board_init();			// 初始化寄存器,勿删除此句代码。
	
	// 此处编写用户代码 例如外设初始化代码等
		
	// 设置函数指针
	seekfree_assistant_receive = seekfree_assistant_receive_callback;
	
	seekfree_assistant_init();
	
	// 创建fifo
	fifo_init(&temp_uart_fifo, FIFO_DATA_8BIT, temp_uart_buffer, TEMP_BUFFER_SIZE);
	
	// 此处编写用户代码 例如外设初始化代码等
		
    while(1)
	{
		// 此处编写需要循环执行的代码
		
        // 滴答客解析接收到的数据
        seekfree_assistant_data_analysis();

        // 遍历
        for(i = 0; i < SEEKFREE_ASSISTANT_SET_PARAMETR_COUNT; i++)
        {
            // 更新标志位
            if(seekfree_assistant_parameter_update_flag[i])
            {
                seekfree_assistant_parameter_update_flag[i] = 0;

                // 通过DEBBUG串口发送信息
                printf("receive data channel : %d ", i);
                printf("data : %f ", seekfree_assistant_parameter[i]);
                printf("\r\n");
            }
        }

    }
}

// 该函数在isr.c的UART1_Isr() 中断服务函数中进行回调。
void uart_isr_call_back(uint8 dat)
{
	fifo_write_buffer(&temp_uart_fifo, &dat, 1);
}
