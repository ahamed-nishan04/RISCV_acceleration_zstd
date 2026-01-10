module BinaryCAM #(
    parameter ADDR_WIDTH = 6,      // 2^6 = 64 locations
    parameter DATA_WIDTH = 32      // Each entry is 32 bits
)(
    input  wire                   clk,
    input  wire                   rst_n,
    input  wire                   write_en,
    input  wire [ADDR_WIDTH-1:0]  write_addr,
    input  wire [DATA_WIDTH-1:0]  write_data,
    input  wire [DATA_WIDTH-1:0]  search_data,
    output reg                    match_found,
    output reg  [ADDR_WIDTH-1:0]  match_addr
);

    // Memory array: 64 entries of 32 bits each
    reg [DATA_WIDTH-1:0] cam_mem [0:(1<<ADDR_WIDTH)-1];
    
    integer i;

    // --- Synchronous Write Logic ---
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            for (i = 0; i < (1<<ADDR_WIDTH); i = i + 1) begin
                cam_mem[i] <= {DATA_WIDTH{1'b0}};
            end
        end else if (write_en) begin
            cam_mem[write_addr] <= write_data;
        end
    end

    // --- Combinational Search Logic ---
    // In hardware, this synthesizes to a parallel comparator for every row.
    always @(*) begin
        match_found = 1'b0;
        match_addr  = {ADDR_WIDTH{1'b0}};
        
        // Priority Encoder: If multiple matches exist, it returns the highest index
        for (i = 0; i < (1<<ADDR_WIDTH); i = i + 1) begin
            if (cam_mem[i] == search_data) begin
                match_found = 1'b1;
                match_addr  = i[ADDR_WIDTH-1:0];
            end
        end
    end

endmodule
