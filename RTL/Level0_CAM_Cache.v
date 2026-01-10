module Level0_CAM_Cache #(
    parameter ENTRIES = 128,
    parameter HASH_WIDTH = 32,
    parameter ADDR_WIDTH = 32
) (
    input  wire                  clk,
    input  wire                  rst_n,
    input  wire [HASH_WIDTH-1:0] search_hash,
    output reg                   hit,
    output reg  [ADDR_WIDTH-1:0] hit_ptr,
    input  wire                  update_en,
    input  wire [HASH_WIDTH-1:0] update_hash,
    input  wire [ADDR_WIDTH-1:0] update_ptr
);
  reg     [HASH_WIDTH-1:0] hash_array  [0:ENTRIES-1];
  reg     [ADDR_WIDTH-1:0] ptr_array   [0:ENTRIES-1];
  reg     [   ENTRIES-1:0] valid_bits;
  reg     [           6:0] replace_ptr;

  integer                  i;
  always @(*) begin
    hit = 1'b0;
    hit_ptr = 0;
    for (i = 0; i < ENTRIES; i = i + 1) begin
      if (valid_bits[i] && (hash_array[i] == search_hash)) begin
        hit = 1'b1;
        hit_ptr = ptr_array[i];
      end
    end
  end

  always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      valid_bits  <= 0;
      replace_ptr <= 0;
    end else if (update_en) begin
      hash_array[replace_ptr] <= update_hash;
      ptr_array[replace_ptr] <= update_ptr;
      valid_bits[replace_ptr] <= 1'b1;
      replace_ptr <= replace_ptr + 1;
    end
  end
endmodule

