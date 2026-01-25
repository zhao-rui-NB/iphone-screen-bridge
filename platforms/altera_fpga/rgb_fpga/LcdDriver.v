module LcdDriver(
    input pclk,              // 系統時鐘
    input rst_n,           // 系統重置，低有效

    // LCD 顯示信號
    output reg hs,          // 水平同步信號
    output reg vs,          // 垂直同步信號
    output reg den,         // 資料啟用信號
    output reg [23:0] rgb,  // 24-bit RGB 輸出

    // pixel data input
    output pixel_request, // 像素請求信號
    output [10:0] pixel_x,   // 像素的 x 坐標
    output [10:0] pixel_y,    // 像素的 y 坐標
    output [10:0] max_x,   // 像素的 x 坐標
    output [10:0] max_y,    // 像素的 y 坐標
    
    input [23:0] pixel_data // 像素數據輸入

);

    // 時間參數設定
    // parameter H_SYNC_CYCLES = 96;   // 水平同步周期
    // parameter H_BACK_PORCH = 48;    // 水平後膺
    // parameter H_ACTIVE_VIDEO = 640; // 水平有效顯示區
    // parameter H_FRONT_PORCH = 16;   // 水平前膺
    // parameter V_SYNC_CYCLES = 2;    // 垂直同步周期
    // parameter V_BACK_PORCH = 33;    // 垂直後膺
    // parameter V_ACTIVE_VIDEO = 480; // 垂直有效顯示區
    // parameter V_FRONT_PORCH = 10;   // 垂直前膺


    // test
    // parameter H_SYNC_CYCLES     = 2;   // 水平同步周期
    // parameter H_BACK_PORCH      = 3;    // 水平後膺
    // parameter H_ACTIVE_VIDEO    = 4;  // 水平有效顯示區
    // parameter H_FRONT_PORCH     = 5;   // 水平前膺
    
    // parameter V_SYNC_CYCLES     = 2;    // 垂直同步周期
    // parameter V_BACK_PORCH      = 3;    // 垂直後膺
    // parameter V_ACTIVE_VIDEO    = 4; // 垂直有效顯示區
    // parameter V_FRONT_PORCH     = 5;   // 垂直前膺


    //IP6 1334x750, H:758, V:1541, pclk= 758*1541*60hz = 70.085 MHZ
    // parameter H_SYNC_CYCLES     = 6;   // 水平同步周期
    // parameter H_BACK_PORCH      = 0;    // 水平後膺
    // parameter H_ACTIVE_VIDEO    = 750;  // 水平有效顯示區
    // parameter H_FRONT_PORCH     = 2;   // 水平前膺
    
    // parameter V_SYNC_CYCLES     = 3;    // 垂直同步周期
    // parameter V_BACK_PORCH      = 4;    // 垂直後膺
    // parameter V_ACTIVE_VIDEO    = 1334; // 垂直有效顯示區
    // parameter V_FRONT_PORCH     = 200;   // 垂直前膺


    //IP7 1334x750, H:776 , V:1540, TOUCH OK
    // parameter H_SYNC_CYCLES     = 3;   // 水平同步周期
    // parameter H_BACK_PORCH      = 3;    // 水平後膺
    // parameter H_ACTIVE_VIDEO    = 750;  // 水平有效顯示區
    // parameter H_FRONT_PORCH     = 50;   // 水平前膺
    
    // parameter V_SYNC_CYCLES     = 3;    // 垂直同步周期
    // parameter V_BACK_PORCH      = 3;    // 垂直後膺
    // parameter V_ACTIVE_VIDEO    = 1334; // 垂直有效顯示區
    // // parameter V_FRONT_PORCH     = 200;   // 垂直前膺
    // parameter V_FRONT_PORCH     = 500;   // 垂直前膺


    //IP7 1334x750, H:776 , V:1540, pclk= 776*1540*60hz = 71.7 MHZ
    // parameter H_SYNC_CYCLES     = 3;   // 水平同步周期
    // parameter H_BACK_PORCH      = 3;    // 水平後膺
    // parameter H_ACTIVE_VIDEO    = 750;  // 水平有效顯示區
    // parameter H_FRONT_PORCH     = 40;   // 水平前膺
    
    // parameter V_SYNC_CYCLES     = 3;    // 垂直同步周期
    // parameter V_BACK_PORCH      = 3;    // 垂直後膺
    // parameter V_ACTIVE_VIDEO    = 1334; // 垂直有效顯示區
    // parameter V_FRONT_PORCH     = 500;   // 垂直前膺

    // 測試 hdmi
    parameter H_SYNC_CYCLES     = 3;   // 水平同步周期
    parameter H_BACK_PORCH      = 3;    // 水平後膺
    parameter H_ACTIVE_VIDEO    = 750;  // 水平有效顯示區
    parameter H_FRONT_PORCH     = 40;   // 水平前膺
    
    parameter V_SYNC_CYCLES     = 3;    // 垂直同步周期
    parameter V_BACK_PORCH      = 3;    // 垂直後膺
    parameter V_ACTIVE_VIDEO    = 1334; // 垂直有效顯示區
    parameter V_FRONT_PORCH     = 10;   // 垂直前膺

    // 計數器
    reg [10:0] h_count = 0;    // 水平計數器
    reg [10:0] v_count = 0;     // 垂直計數器

    wire [10:0] h_total = H_SYNC_CYCLES + H_BACK_PORCH + H_ACTIVE_VIDEO + H_FRONT_PORCH; // 水平總周期
    wire [10:0] v_total = V_SYNC_CYCLES + V_BACK_PORCH + V_ACTIVE_VIDEO + V_FRONT_PORCH; // 垂直總周期

    wire [10:0] next_h_count = (h_count < (h_total - 1)) ? h_count + 1 : 0; // 水平計數器
    wire [10:0] next_v_count = (h_count == (h_total - 1)) ? (v_count < (v_total - 1) ? v_count + 1 : 0) : v_count; // 垂直計數器

    // 水平與垂直同步控制
    always @(negedge pclk or negedge rst_n) begin
        if(!rst_n) begin
            h_count <= 0;
            v_count <= 0;
        end else begin
            h_count <= next_h_count; // 更新水平計數器
            v_count <= next_v_count; // 更新垂直計數器
        end
    end

    // 水平同步信號生成 HS
    always @(negedge pclk or negedge rst_n) begin
        if(!rst_n) begin
            hs <= 1; // 初始狀態為高電平
        end else begin
            // 當計數器在同步周期內時，輸出低電平
            if (h_count < H_SYNC_CYCLES) begin
                hs <= 0;
            end else begin
                hs <= 1;
            end
        end
    end

    // 垂直同步信號生成 VS
    always @(negedge pclk or negedge rst_n) begin
        if(!rst_n) begin
            vs <= 1; // 初始狀態為高電平
        end else begin
            if (v_count < V_SYNC_CYCLES) begin
                vs <= 0;
            end else begin
                vs <= 1;
            end
        end
    end

    // 資料啟用信號生成 DEN
    always @(negedge pclk or negedge rst_n) begin
        if(!rst_n) begin
            den <= 0; // 初始狀態為低電平
        end else begin
            if (h_count >= (H_SYNC_CYCLES + H_BACK_PORCH) && h_count < (H_SYNC_CYCLES + H_BACK_PORCH + H_ACTIVE_VIDEO) && v_count >= (V_SYNC_CYCLES + V_BACK_PORCH) && v_count < (V_SYNC_CYCLES + V_BACK_PORCH + V_ACTIVE_VIDEO)) begin
                den <= 1;
            end else begin
                den <= 0;
            end
        end
    end

    wire is_next_active_video = (
        next_h_count >= (H_SYNC_CYCLES + H_BACK_PORCH) && 
        next_h_count < (H_SYNC_CYCLES + H_BACK_PORCH + H_ACTIVE_VIDEO) && 
        next_v_count >= (V_SYNC_CYCLES + V_BACK_PORCH) && 
        next_v_count < (V_SYNC_CYCLES + V_BACK_PORCH + V_ACTIVE_VIDEO)
    );
    wire [10:0] h_pos = h_count - (H_SYNC_CYCLES + H_BACK_PORCH); // 當前水平位置
    wire [10:0] v_pos = v_count - (V_SYNC_CYCLES + V_BACK_PORCH); // 當前垂直位置


    // set the request signal to 1 when the next pixel is in the active area
    assign pixel_request = is_next_active_video;
    assign pixel_x = next_h_count - (H_SYNC_CYCLES + H_BACK_PORCH); // 當前像素的 x 坐標
    assign pixel_y = next_v_count - (V_SYNC_CYCLES + V_BACK_PORCH); // 當前像素的 y 坐標
    assign max_x = H_ACTIVE_VIDEO;
    assign max_y = V_ACTIVE_VIDEO;

    always @(negedge pclk or negedge rst_n) begin
        if(!rst_n) begin
            rgb <= 24'b0; // 初始狀態為黑色
        end else begin
            rgb <= pixel_data; // 當前像素的 RGB 數據    
            // rgb <= 24'h00FF00; // 當前像素的 RGB 數據    
        end
    end

    // 24-bit RGB 測試信號（簡單的檢測顏色條）
    /*
    always @(negedge pclk or negedge rst_n) begin
        if(!rst_n) begin
            rgb <= 24'b0; // 初始狀態為黑色
        end else begin
            // calc now active pixel x y position
            if (active_video) begin
                // rgb <= 24'h00FF00;
                // // 根據當前位置生成不同顏色的條紋
                // if (v_pos < V_ACTIVE_VIDEO / 3) begin
                //     rgb <= 24'hFF0000; // 紅色
                // end else if (v_pos < 2 * V_ACTIVE_VIDEO / 3) begin
                //     rgb <= 24'h00FF00; // 綠色
                // end else begin
                //     rgb <= 24'h0000FF; // 藍色
                // end

                if (h_pos < H_ACTIVE_VIDEO / 3) begin
                    rgb <= 24'hFF0000; // 紅色
                end else if (h_pos < 2 * H_ACTIVE_VIDEO / 3) begin
                    rgb <= 24'h00FF00; // 綠色
                end else begin
                    rgb <= 24'h0000FF; // 藍色
                end



            end else begin
                rgb <= 24'b0; // 非有效顯示區域為黑色
            end
        end
    end
    */

endmodule
