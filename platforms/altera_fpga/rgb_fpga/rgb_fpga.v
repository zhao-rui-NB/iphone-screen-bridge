module rgb_fpga(
    input clk,              // 系統時鐘 50M
    input rst_n,           // 系統重置，低有效

    output lcd_reset,    // LCD 重置信號
    output lcd_bl,       // LCD 背光控制信號，高有效

    output pclk,         // 像素時鐘
    output vs,          // 垂直同步信號
    output hs,          // 水平同步信號
    output den,         // 資料啟用信號
    output [23:0] rgb,  // 24-bit RGB 輸出


    // key
    input btn_next_img // 鍵盤輸入信號
);


    assign lcd_reset = 1'b0; // no reset
    assign lcd_bl = 1'b1; // backlight always on

    // assign pclk = clk; // 直接使用系統時鐘作為像素時鐘
    assign pclk = clk_70m; 

    pll	pll_70m (
	    .areset ( !rst_n ),
	    .inclk0 ( clk ),
	    .c0 ( clk_70m )
	);


    
    wire pixel_request;
    wire [10:0] pixel_x; // 像素的 x 坐標
    wire [10:0] pixel_y; // 像素的 y 坐標
    wire [10:0] max_x; // 畫面最大 x 坐標
    wire [10:0] max_y; // 畫面最大 y 坐標
    wire [23:0] pixel_data; // 像素數據輸入

    LcdDriver lcd_driver(
        .pclk(pclk),
        .rst_n(rst_n),

        .hs(hs),
        .vs(vs),
        .den(den),
        .rgb(rgb),

        .pixel_request(pixel_request),
        .pixel_x(pixel_x),
        .pixel_y(pixel_y),
        .max_x(max_x),
        .max_y(max_y),

        .pixel_data(pixel_data)
    );

    DisplaySignal display_signal(
        .pclk(pclk),
        .rst_n(rst_n),

        .pixel_request(pixel_request),
        .pixel_x(pixel_x),
        .pixel_y(pixel_y),
        .max_x(max_x),
        .max_y(max_y),

        .pixel_data(pixel_data),

        // key
        .btn_next_img(btn_next_img) // 鍵盤輸入信號
    );
endmodule
