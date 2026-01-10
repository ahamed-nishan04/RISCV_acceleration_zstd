module FourWayNodeSearcher #(
    parameter HASH_WIDTH = 32,
    parameter ADDR_WIDTH = 32
) (
    input  wire [HASH_WIDTH-1:0] target_hash,
    input  wire [ADDR_WIDTH-1:0] current_offset,
    input  wire [ADDR_WIDTH-1:0] window_size,
    input  wire [         511:0] dram_node_data,
    output reg  [ADDR_WIDTH-1:0] next_node_ptr,
    output reg                   match_found,
    output reg  [ADDR_WIDTH-1:0] match_address
);
  wire [31:0] k0 = dram_node_data[31:0], k1 = dram_node_data[63:32], k2 = dram_node_data[95:64];
  wire [31:0] p0 = dram_node_data[127:96], p1 = dram_node_data[159:128], p2 = dram_node_data[191:160], p3 = dram_node_data[223:192];
  wire [31:0] o0 = dram_node_data[255:224], o1 = dram_node_data[287:256], o2 = dram_node_data[319:288];

  always @(*) begin
    match_found   = 0;
    match_address = 0;
    next_node_ptr = 0;
    if (target_hash == k0 && (current_offset - o0 <= window_size)) begin
      match_found   = 1;
      match_address = o0;
    end else if (target_hash == k1 && (current_offset - o1 <= window_size)) begin
      match_found   = 1;
      match_address = o1;
    end else if (target_hash == k2 && (current_offset - o2 <= window_size)) begin
      match_found   = 1;
      match_address = o2;
    end else begin
      if (target_hash < k0) next_node_ptr = p0;
      else if (target_hash < k1) next_node_ptr = p1;
      else if (target_hash < k2) next_node_ptr = p2;
      else next_node_ptr = p3;
    end
  end
endmodule

