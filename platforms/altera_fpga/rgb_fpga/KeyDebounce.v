module KeyDebounce (
    input clk,
    input rst_n,
    
    input key_in, // 鍵盤輸入信號
    output reg key_pulse // 鍵盤輸出信號
);


// clk 50Mhz
// 50M*20ms = 1_000_000
parameter CNT_MAX = 1_000_000;

reg [19:0] cnt_20ms ; //计数器


//cnt_20ms:如果时钟的上升沿检测到外部按键输入的值为低电平时，计数器开始计数
always@(posedge clk or negedge rst_n)begin
    if(!rst_n)
        cnt_20ms <= 0;
    else if(key_in == 1'b1)
        cnt_20ms <= 0;
    else if(cnt_20ms == CNT_MAX && key_in == 1'b0)
        cnt_20ms <= cnt_20ms;
    else
        cnt_20ms <= cnt_20ms + 1'b1;
end

always@(posedge clk or negedge rst_n)begin
    if(!rst_n)
        key_pulse <= 0;
    else if(cnt_20ms == CNT_MAX-1 && key_in == 1'b0)
        key_pulse <= 1'b1; // 产生一个脉冲信号
    else
        key_pulse <= 1'b0; // 其他情况输出低电平
end

endmodule