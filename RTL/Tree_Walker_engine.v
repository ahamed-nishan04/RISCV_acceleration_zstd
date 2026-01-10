module Tree_Walker_Engine #(
    parameter HASH_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter MAX_STEPS  = 32
) (
    input clk,
    rst_n,
    input task_valid,
    input [HASH_WIDTH-1:0] task_hash,
    input [ADDR_WIDTH-1:0] task_offset,
    output reg ready,
    output reg match_found,
    output reg [ADDR_WIDTH-1:0] best_match_addr,

    output reg [ADDR_WIDTH-1:0] dram_addr,
    output reg dram_req,
    input [511:0] dram_data,
    input dram_valid
);
  wire cam_hit;
  wire [ADDR_WIDTH-1:0] cam_ptr;
  wire [ADDR_WIDTH-1:0] nxt_ptr, m_addr;
  wire m_found;

  reg [5:0] step_count;
  reg found_at_least_one;

  Level0_CAM_Cache #(128, HASH_WIDTH, ADDR_WIDTH) cam (
      .clk(clk),
      .rst_n(rst_n),
      .search_hash(task_hash),
      .hit(cam_hit),
      .hit_ptr(cam_ptr),
      .update_en(match_found && found_at_least_one),
      .update_hash(task_hash),
      .update_ptr(best_match_addr)
  );

  FourWayNodeSearcher #(HASH_WIDTH, ADDR_WIDTH) searcher (
      .target_hash(task_hash),
      .current_offset(task_offset),
      .window_size(32'h0010_0000),  // 1MB Window
      .dram_node_data(dram_data),
      .next_node_ptr(nxt_ptr),
      .match_found(m_found),
      .match_address(m_addr)
  );

  reg [1:0] state;
  always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state <= 0;
      ready <= 1;
      dram_req <= 0;
      match_found <= 0;
      step_count <= 0;
      best_match_addr <= 0;
      found_at_least_one <= 0;
    end else begin
      case (state)
        0:
        if (task_valid) begin
          dram_addr <= cam_hit ? cam_ptr : 32'h1000;
          dram_req <= 1;
          ready <= 0;
          state <= 1;
          match_found <= 0;
          step_count <= 0;
          found_at_least_one <= 0;
          best_match_addr <= 0;
        end
        1:
        if (dram_valid) begin
          dram_req <= 0;
          step_count <= step_count + 1;
          state <= 2;
        end
        2: begin
          if (m_found) begin
            best_match_addr <= m_addr;
            found_at_least_one <= 1;
          end
          if (step_count >= MAX_STEPS || nxt_ptr == 0) begin
            match_found <= 1;
            ready <= 1;
            state <= 0;
          end else begin
            dram_addr <= nxt_ptr;
            dram_req <= 1;
            state <= 1;
          end
        end
      endcase
    end
  end
endmodule

