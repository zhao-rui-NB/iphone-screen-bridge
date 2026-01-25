module DisplaySignal(
    input pclk,              // 系統時鐘
    input rst_n,           // 系統重置，低有效

    input pixel_request, // 像素請求信號
    input [10:0] pixel_x,   // 像素的 x 坐標
    input [10:0] pixel_y,   // 像素的 y 坐標
    input [10:0] max_x,      // 畫面最大 x 坐標
    input [10:0] max_y,      // 畫面最大 y 坐標

    output reg [23:0] pixel_data, // 像素數據輸出

    // key 
    input btn_next_img // 鍵盤輸入信號

);

// color 
parameter RED = 24'hFF0000; // Red color
parameter GREEN = 24'h00FF00; // Green color
parameter BLUE = 24'h0000FF; // Blue color
parameter WHITE = 24'hFFFFFF; // White color
parameter BLACK = 24'h000000; // Black color 

parameter colors = 5;


KeyDebounce key_debounce (
    .clk(pclk),
    .rst_n(rst_n),

    .key_in(btn_next_img), // 鍵盤輸入信號
    .key_pulse(next_img) 
);

// reg [3:0]img_id = 0;
reg [1:0]img_id = 0;


always @(posedge pclk or negedge rst_n)begin
    if(!rst_n)begin
        img_id <= 0;
    end else begin
        if(next_img)begin
            if(img_id == 4) begin
                img_id <= 0; // 循環回到第一張圖片
            end else
            begin
                img_id <= img_id + 1; // 切換到下一張圖片
            end
        end else begin
            img_id <= img_id;
        end
    end
end


// grid 1 
reg [23:0] grid1_color;
// if x or y mod 10 == 0 draw black line 
// else fill with red blue green white color in order if line is odd reverse the color order

always @(*) begin
    if (pixel_x % 100 <= 20 || pixel_y % 100 <= 20) begin
        grid1_color = BLACK; // 畫黑色線條
    end else begin

        // gx = pixel_x / 50; // 畫面 x 坐標除以 50
        // gy = pixel_y / 50; // 畫面 y 坐標除以 50
        case( ((pixel_x / 100)+ (pixel_y / 100)) % 9)
            0: grid1_color = 24'hFF0000;
            1: grid1_color = 24'h00FF00;
            2: grid1_color = 24'h0000FF;
            3: grid1_color = 24'h00FFFF;
            4: grid1_color = 24'hFF00FF;
            5: grid1_color = 24'hFFFF00;
            6: grid1_color = 24'h7f0000; 
            7: grid1_color = 24'h007f00; 
            8: grid1_color = 24'h00007f;
            default: grid1_color = BLACK; // default color
        endcase

    end
end

    


always @(negedge pclk or negedge rst_n)begin
    if(!rst_n)begin
        pixel_data <= 0;
    end else begin
        if(img_id == 0)begin
            if(pixel_request)begin
                case(pixel_x / (max_x / colors))
                        0: pixel_data <= RED;
                        1: pixel_data <= GREEN;
                        2: pixel_data <= BLUE;
                        3: pixel_data <= WHITE;
                        4: pixel_data <= BLACK;
                        default: pixel_data <= BLACK; // default color
                endcase
            end else begin
                pixel_data <= BLACK; // default color
            end
        end else if(img_id == 1)begin // red
            if(pixel_request)begin
                pixel_data <= RED;
            end else begin
                pixel_data <= BLACK; // default color
            end
        end else if(img_id == 2)begin // green
            if(pixel_request)begin
                pixel_data <= GREEN;
            end else begin
                pixel_data <= BLACK; // default color
            end
        end else if(img_id == 3)begin // blue
            if(pixel_request)begin
                pixel_data <= grid1_color;
            end else begin
                pixel_data <= BLACK; // default color
            end
        end else if(img_id == 4)begin // grid1
            if(pixel_request)begin
                pixel_data <= grid1_color;
            end else begin
                pixel_data <= BLACK; // default color
            end
        end

    end
    
end

endmodule