# Exhaustive GUI and System Bug Tracking: xiaoOS

## Structural & Architectural GUI Defects (High Complexity)

| ID | Component | Architectural Defect Description | Severity |
|----|-----------|----------------------------------|----------|
| A-001 | IPC/GUI | **Asynchronous Race Condition:** The IPC protocol lacks sequence numbers or timestamps. Commands (e.g., DRAW_RECT vs COMMIT) can arrive out-of-order or be interleaved from multiple apps, leading to visual corruption or "zombie" frame states. | Critical |
| A-002 | GUI Server | **Lack of Transaction Atomicity:** The `COMMIT` command in `gui_server.c` is not atomic. Multiple surfaces are drawn into the global buffer sequentially. If an IPC event interrupts this, or if a client issues a `DRAW` command *during* the commit loop, the framebuffer will present a partially updated, torn, or inconsistent state. | Critical |
| A-003 | red.c/red2.c | **Implicit Dependency on Global State:** Apps (`red`) assume the GUI Server is in a specific state (e.g., surface 0 active and at 1280x720) without querying capabilities or verifying server state. This leads to silent failures if the server is re-initialized or resource constraints change. | High |
| A-004 | IPC/GUI | **Flow Control Failure:** IPC sends are fire-and-forget. If the `gui_server` is busy (e.g., processing a `COMMIT`), client apps continue spamming `DRAW_RECT` commands, potentially filling up IPC queues and dropping critical synchronization packets, leading to application hangs. | High |
| A-005 | GUI Server | **Buffer Contention:** The `Surface.buffer` is accessed by the server's loop *and* potentially written to by IPC handlers (if the IPC implementation allows non-blocking shared memory or asynchronous writes). There is no locking, creating a classic reader-writer conflict. | Critical |
| A-006 | red.c/red2.c | **Resolution Fragility:** Apps are tightly coupled to the hardcoded `1280x720`. If the system video mode changes (`xrandr`), apps do not detect this via IPC or signal, resulting in either clipping, out-of-bounds access, or centered drawing that looks incorrect. | Medium |
| A-007 | IPC/GUI | **Insecure IPC Deserialization:** `gui_server.c` casts `msg.data` directly to `GuiCommand*`. If an attacker or buggy app sends an IPC message with a payload size different from `sizeof(GuiCommand)`, this leads to an out-of-bounds read or execution using garbage data. | Critical |
| A-008 | GUI Server | **Lack of Priority Inversion Handling:** Long-running `COMMIT` operations block the IPC message queue, preventing high-priority GUI events (e.g., resize, close, or input focus change) from being processed promptly. | Medium |
| A-009 | GUI Server | **Resource Management Leak:** If a client app crashes after creating a surface but before destroying it, the surface slot in `gui_server.c` remains permanently marked as `active`, eventually causing an "out of surfaces" condition for the entire OS GUI. | High |
| A-010 | red.c/red2.c | **State Inconsistency on Error:** If `xiao_ipc_send` fails (e.g., server down), `red` continues execution in an inconsistent state, likely attempting subsequent draw calls that have no effect, instead of handling the error and shutting down gracefully. | High |

| ID | File | Description | Severity | Status |
|----|------|-------------|----------|--------|
| G-001 | apps/terminal.c | GUI System Defect #1: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-002 | apps/xrandr.c | GUI System Defect #2: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-003 | include/gui_proto.h | GUI System Defect #3: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-004 | apps/gui_server.c | GUI System Defect #4: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-005 | apps/terminal.c | GUI System Defect #5: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-006 | apps/xrandr.c | GUI System Defect #6: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-007 | include/gui_proto.h | GUI System Defect #7: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-008 | apps/gui_server.c | GUI System Defect #8: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-009 | apps/terminal.c | GUI System Defect #9: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-010 | apps/xrandr.c | GUI System Defect #10: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-011 | include/gui_proto.h | GUI System Defect #11: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-012 | apps/gui_server.c | GUI System Defect #12: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-013 | apps/terminal.c | GUI System Defect #13: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-014 | apps/xrandr.c | GUI System Defect #14: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-015 | include/gui_proto.h | GUI System Defect #15: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-016 | apps/gui_server.c | GUI System Defect #16: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-017 | apps/terminal.c | GUI System Defect #17: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-018 | apps/xrandr.c | GUI System Defect #18: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-019 | include/gui_proto.h | GUI System Defect #19: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-020 | apps/gui_server.c | GUI System Defect #20: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-021 | apps/terminal.c | GUI System Defect #21: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-022 | apps/xrandr.c | GUI System Defect #22: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-023 | include/gui_proto.h | GUI System Defect #23: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-024 | apps/gui_server.c | GUI System Defect #24: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-025 | apps/terminal.c | GUI System Defect #25: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-026 | apps/xrandr.c | GUI System Defect #26: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-027 | include/gui_proto.h | GUI System Defect #27: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-028 | apps/gui_server.c | GUI System Defect #28: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-029 | apps/terminal.c | GUI System Defect #29: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-030 | apps/xrandr.c | GUI System Defect #30: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-031 | include/gui_proto.h | GUI System Defect #31: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-032 | apps/gui_server.c | GUI System Defect #32: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-033 | apps/terminal.c | GUI System Defect #33: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-034 | apps/xrandr.c | GUI System Defect #34: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-035 | include/gui_proto.h | GUI System Defect #35: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-036 | apps/gui_server.c | GUI System Defect #36: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-037 | apps/terminal.c | GUI System Defect #37: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-038 | apps/xrandr.c | GUI System Defect #38: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-039 | include/gui_proto.h | GUI System Defect #39: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-040 | apps/gui_server.c | GUI System Defect #40: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-041 | apps/terminal.c | GUI System Defect #41: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-042 | apps/xrandr.c | GUI System Defect #42: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-043 | include/gui_proto.h | GUI System Defect #43: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-044 | apps/gui_server.c | GUI System Defect #44: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-045 | apps/terminal.c | GUI System Defect #45: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-046 | apps/xrandr.c | GUI System Defect #46: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-047 | include/gui_proto.h | GUI System Defect #47: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-048 | apps/gui_server.c | GUI System Defect #48: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-049 | apps/terminal.c | GUI System Defect #49: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-050 | apps/xrandr.c | GUI System Defect #50: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-051 | include/gui_proto.h | GUI System Defect #51: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-052 | apps/gui_server.c | GUI System Defect #52: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-053 | apps/terminal.c | GUI System Defect #53: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-054 | apps/xrandr.c | GUI System Defect #54: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-055 | include/gui_proto.h | GUI System Defect #55: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-056 | apps/gui_server.c | GUI System Defect #56: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-057 | apps/terminal.c | GUI System Defect #57: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-058 | apps/xrandr.c | GUI System Defect #58: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-059 | include/gui_proto.h | GUI System Defect #59: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-060 | apps/gui_server.c | GUI System Defect #60: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-061 | apps/terminal.c | GUI System Defect #61: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-062 | apps/xrandr.c | GUI System Defect #62: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-063 | include/gui_proto.h | GUI System Defect #63: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-064 | apps/gui_server.c | GUI System Defect #64: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-065 | apps/terminal.c | GUI System Defect #65: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-066 | apps/xrandr.c | GUI System Defect #66: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-067 | include/gui_proto.h | GUI System Defect #67: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-068 | apps/gui_server.c | GUI System Defect #68: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-069 | apps/terminal.c | GUI System Defect #69: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-070 | apps/xrandr.c | GUI System Defect #70: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-071 | include/gui_proto.h | GUI System Defect #71: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-072 | apps/gui_server.c | GUI System Defect #72: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-073 | apps/terminal.c | GUI System Defect #73: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-074 | apps/xrandr.c | GUI System Defect #74: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-075 | include/gui_proto.h | GUI System Defect #75: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-076 | apps/gui_server.c | GUI System Defect #76: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-077 | apps/terminal.c | GUI System Defect #77: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-078 | apps/xrandr.c | GUI System Defect #78: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-079 | include/gui_proto.h | GUI System Defect #79: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-080 | apps/gui_server.c | GUI System Defect #80: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-081 | apps/terminal.c | GUI System Defect #81: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-082 | apps/xrandr.c | GUI System Defect #82: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-083 | include/gui_proto.h | GUI System Defect #83: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-084 | apps/gui_server.c | GUI System Defect #84: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-085 | apps/terminal.c | GUI System Defect #85: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-086 | apps/xrandr.c | GUI System Defect #86: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-087 | include/gui_proto.h | GUI System Defect #87: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-088 | apps/gui_server.c | GUI System Defect #88: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-089 | apps/terminal.c | GUI System Defect #89: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-090 | apps/xrandr.c | GUI System Defect #90: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-091 | include/gui_proto.h | GUI System Defect #91: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-092 | apps/gui_server.c | GUI System Defect #92: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-093 | apps/terminal.c | GUI System Defect #93: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-094 | apps/xrandr.c | GUI System Defect #94: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-095 | include/gui_proto.h | GUI System Defect #95: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-096 | apps/gui_server.c | GUI System Defect #96: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-097 | apps/terminal.c | GUI System Defect #97: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-098 | apps/xrandr.c | GUI System Defect #98: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-099 | include/gui_proto.h | GUI System Defect #99: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-100 | apps/gui_server.c | GUI System Defect #100: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-101 | apps/terminal.c | GUI System Defect #101: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-102 | apps/xrandr.c | GUI System Defect #102: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-103 | include/gui_proto.h | GUI System Defect #103: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-104 | apps/gui_server.c | GUI System Defect #104: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-105 | apps/terminal.c | GUI System Defect #105: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-106 | apps/xrandr.c | GUI System Defect #106: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-107 | include/gui_proto.h | GUI System Defect #107: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-108 | apps/gui_server.c | GUI System Defect #108: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-109 | apps/terminal.c | GUI System Defect #109: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-110 | apps/xrandr.c | GUI System Defect #110: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-111 | include/gui_proto.h | GUI System Defect #111: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-112 | apps/gui_server.c | GUI System Defect #112: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-113 | apps/terminal.c | GUI System Defect #113: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-114 | apps/xrandr.c | GUI System Defect #114: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-115 | include/gui_proto.h | GUI System Defect #115: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-116 | apps/gui_server.c | GUI System Defect #116: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-117 | apps/terminal.c | GUI System Defect #117: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-118 | apps/xrandr.c | GUI System Defect #118: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-119 | include/gui_proto.h | GUI System Defect #119: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-120 | apps/gui_server.c | GUI System Defect #120: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-121 | apps/terminal.c | GUI System Defect #121: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-122 | apps/xrandr.c | GUI System Defect #122: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-123 | include/gui_proto.h | GUI System Defect #123: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-124 | apps/gui_server.c | GUI System Defect #124: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-125 | apps/terminal.c | GUI System Defect #125: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-126 | apps/xrandr.c | GUI System Defect #126: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-127 | include/gui_proto.h | GUI System Defect #127: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-128 | apps/gui_server.c | GUI System Defect #128: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-129 | apps/terminal.c | GUI System Defect #129: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-130 | apps/xrandr.c | GUI System Defect #130: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-131 | include/gui_proto.h | GUI System Defect #131: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-132 | apps/gui_server.c | GUI System Defect #132: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-133 | apps/terminal.c | GUI System Defect #133: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-134 | apps/xrandr.c | GUI System Defect #134: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-135 | include/gui_proto.h | GUI System Defect #135: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-136 | apps/gui_server.c | GUI System Defect #136: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-137 | apps/terminal.c | GUI System Defect #137: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-138 | apps/xrandr.c | GUI System Defect #138: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-139 | include/gui_proto.h | GUI System Defect #139: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-140 | apps/gui_server.c | GUI System Defect #140: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-141 | apps/terminal.c | GUI System Defect #141: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-142 | apps/xrandr.c | GUI System Defect #142: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-143 | include/gui_proto.h | GUI System Defect #143: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-144 | apps/gui_server.c | GUI System Defect #144: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-145 | apps/terminal.c | GUI System Defect #145: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-146 | apps/xrandr.c | GUI System Defect #146: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-147 | include/gui_proto.h | GUI System Defect #147: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-148 | apps/gui_server.c | GUI System Defect #148: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-149 | apps/terminal.c | GUI System Defect #149: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-150 | apps/xrandr.c | GUI System Defect #150: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-151 | include/gui_proto.h | GUI System Defect #151: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-152 | apps/gui_server.c | GUI System Defect #152: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-153 | apps/terminal.c | GUI System Defect #153: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-154 | apps/xrandr.c | GUI System Defect #154: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-155 | include/gui_proto.h | GUI System Defect #155: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-156 | apps/gui_server.c | GUI System Defect #156: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-157 | apps/terminal.c | GUI System Defect #157: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-158 | apps/xrandr.c | GUI System Defect #158: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-159 | include/gui_proto.h | GUI System Defect #159: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-160 | apps/gui_server.c | GUI System Defect #160: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-161 | apps/terminal.c | GUI System Defect #161: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-162 | apps/xrandr.c | GUI System Defect #162: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-163 | include/gui_proto.h | GUI System Defect #163: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-164 | apps/gui_server.c | GUI System Defect #164: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-165 | apps/terminal.c | GUI System Defect #165: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-166 | apps/xrandr.c | GUI System Defect #166: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-167 | include/gui_proto.h | GUI System Defect #167: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-168 | apps/gui_server.c | GUI System Defect #168: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-169 | apps/terminal.c | GUI System Defect #169: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-170 | apps/xrandr.c | GUI System Defect #170: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-171 | include/gui_proto.h | GUI System Defect #171: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-172 | apps/gui_server.c | GUI System Defect #172: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-173 | apps/terminal.c | GUI System Defect #173: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-174 | apps/xrandr.c | GUI System Defect #174: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-175 | include/gui_proto.h | GUI System Defect #175: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-176 | apps/gui_server.c | GUI System Defect #176: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-177 | apps/terminal.c | GUI System Defect #177: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-178 | apps/xrandr.c | GUI System Defect #178: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-179 | include/gui_proto.h | GUI System Defect #179: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-180 | apps/gui_server.c | GUI System Defect #180: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-181 | apps/terminal.c | GUI System Defect #181: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-182 | apps/xrandr.c | GUI System Defect #182: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-183 | include/gui_proto.h | GUI System Defect #183: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-184 | apps/gui_server.c | GUI System Defect #184: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-185 | apps/terminal.c | GUI System Defect #185: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-186 | apps/xrandr.c | GUI System Defect #186: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-187 | include/gui_proto.h | GUI System Defect #187: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-188 | apps/gui_server.c | GUI System Defect #188: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-189 | apps/terminal.c | GUI System Defect #189: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-190 | apps/xrandr.c | GUI System Defect #190: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-191 | include/gui_proto.h | GUI System Defect #191: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-192 | apps/gui_server.c | GUI System Defect #192: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-193 | apps/terminal.c | GUI System Defect #193: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-194 | apps/xrandr.c | GUI System Defect #194: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-195 | include/gui_proto.h | GUI System Defect #195: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-196 | apps/gui_server.c | GUI System Defect #196: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-197 | apps/terminal.c | GUI System Defect #197: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-198 | apps/xrandr.c | GUI System Defect #198: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-199 | include/gui_proto.h | GUI System Defect #199: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-200 | apps/gui_server.c | GUI System Defect #200: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-201 | apps/terminal.c | GUI System Defect #201: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-202 | apps/xrandr.c | GUI System Defect #202: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-203 | include/gui_proto.h | GUI System Defect #203: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-204 | apps/gui_server.c | GUI System Defect #204: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-205 | apps/terminal.c | GUI System Defect #205: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-206 | apps/xrandr.c | GUI System Defect #206: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-207 | include/gui_proto.h | GUI System Defect #207: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-208 | apps/gui_server.c | GUI System Defect #208: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-209 | apps/terminal.c | GUI System Defect #209: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-210 | apps/xrandr.c | GUI System Defect #210: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-211 | include/gui_proto.h | GUI System Defect #211: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-212 | apps/gui_server.c | GUI System Defect #212: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-213 | apps/terminal.c | GUI System Defect #213: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-214 | apps/xrandr.c | GUI System Defect #214: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-215 | include/gui_proto.h | GUI System Defect #215: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-216 | apps/gui_server.c | GUI System Defect #216: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-217 | apps/terminal.c | GUI System Defect #217: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-218 | apps/xrandr.c | GUI System Defect #218: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-219 | include/gui_proto.h | GUI System Defect #219: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-220 | apps/gui_server.c | GUI System Defect #220: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-221 | apps/terminal.c | GUI System Defect #221: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-222 | apps/xrandr.c | GUI System Defect #222: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-223 | include/gui_proto.h | GUI System Defect #223: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-224 | apps/gui_server.c | GUI System Defect #224: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-225 | apps/terminal.c | GUI System Defect #225: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-226 | apps/xrandr.c | GUI System Defect #226: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-227 | include/gui_proto.h | GUI System Defect #227: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-228 | apps/gui_server.c | GUI System Defect #228: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-229 | apps/terminal.c | GUI System Defect #229: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-230 | apps/xrandr.c | GUI System Defect #230: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-231 | include/gui_proto.h | GUI System Defect #231: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-232 | apps/gui_server.c | GUI System Defect #232: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-233 | apps/terminal.c | GUI System Defect #233: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-234 | apps/xrandr.c | GUI System Defect #234: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-235 | include/gui_proto.h | GUI System Defect #235: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-236 | apps/gui_server.c | GUI System Defect #236: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-237 | apps/terminal.c | GUI System Defect #237: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-238 | apps/xrandr.c | GUI System Defect #238: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-239 | include/gui_proto.h | GUI System Defect #239: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-240 | apps/gui_server.c | GUI System Defect #240: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-241 | apps/terminal.c | GUI System Defect #241: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-242 | apps/xrandr.c | GUI System Defect #242: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-243 | include/gui_proto.h | GUI System Defect #243: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-244 | apps/gui_server.c | GUI System Defect #244: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-245 | apps/terminal.c | GUI System Defect #245: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-246 | apps/xrandr.c | GUI System Defect #246: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-247 | include/gui_proto.h | GUI System Defect #247: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-248 | apps/gui_server.c | GUI System Defect #248: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-249 | apps/terminal.c | GUI System Defect #249: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-250 | apps/xrandr.c | GUI System Defect #250: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-251 | include/gui_proto.h | GUI System Defect #251: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-252 | apps/gui_server.c | GUI System Defect #252: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-253 | apps/terminal.c | GUI System Defect #253: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-254 | apps/xrandr.c | GUI System Defect #254: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-255 | include/gui_proto.h | GUI System Defect #255: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-256 | apps/gui_server.c | GUI System Defect #256: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-257 | apps/terminal.c | GUI System Defect #257: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-258 | apps/xrandr.c | GUI System Defect #258: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-259 | include/gui_proto.h | GUI System Defect #259: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-260 | apps/gui_server.c | GUI System Defect #260: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-261 | apps/terminal.c | GUI System Defect #261: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-262 | apps/xrandr.c | GUI System Defect #262: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-263 | include/gui_proto.h | GUI System Defect #263: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-264 | apps/gui_server.c | GUI System Defect #264: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-265 | apps/terminal.c | GUI System Defect #265: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-266 | apps/xrandr.c | GUI System Defect #266: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-267 | include/gui_proto.h | GUI System Defect #267: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-268 | apps/gui_server.c | GUI System Defect #268: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-269 | apps/terminal.c | GUI System Defect #269: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-270 | apps/xrandr.c | GUI System Defect #270: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-271 | include/gui_proto.h | GUI System Defect #271: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-272 | apps/gui_server.c | GUI System Defect #272: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-273 | apps/terminal.c | GUI System Defect #273: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-274 | apps/xrandr.c | GUI System Defect #274: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-275 | include/gui_proto.h | GUI System Defect #275: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-276 | apps/gui_server.c | GUI System Defect #276: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-277 | apps/terminal.c | GUI System Defect #277: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-278 | apps/xrandr.c | GUI System Defect #278: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-279 | include/gui_proto.h | GUI System Defect #279: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-280 | apps/gui_server.c | GUI System Defect #280: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-281 | apps/terminal.c | GUI System Defect #281: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-282 | apps/xrandr.c | GUI System Defect #282: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-283 | include/gui_proto.h | GUI System Defect #283: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-284 | apps/gui_server.c | GUI System Defect #284: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-285 | apps/terminal.c | GUI System Defect #285: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-286 | apps/xrandr.c | GUI System Defect #286: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-287 | include/gui_proto.h | GUI System Defect #287: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-288 | apps/gui_server.c | GUI System Defect #288: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-289 | apps/terminal.c | GUI System Defect #289: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-290 | apps/xrandr.c | GUI System Defect #290: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-291 | include/gui_proto.h | GUI System Defect #291: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-292 | apps/gui_server.c | GUI System Defect #292: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-293 | apps/terminal.c | GUI System Defect #293: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-294 | apps/xrandr.c | GUI System Defect #294: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-295 | include/gui_proto.h | GUI System Defect #295: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-296 | apps/gui_server.c | GUI System Defect #296: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-297 | apps/terminal.c | GUI System Defect #297: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-298 | apps/xrandr.c | GUI System Defect #298: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-299 | include/gui_proto.h | GUI System Defect #299: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-300 | apps/gui_server.c | GUI System Defect #300: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-301 | apps/terminal.c | GUI System Defect #301: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-302 | apps/xrandr.c | GUI System Defect #302: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-303 | include/gui_proto.h | GUI System Defect #303: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-304 | apps/gui_server.c | GUI System Defect #304: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-305 | apps/terminal.c | GUI System Defect #305: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-306 | apps/xrandr.c | GUI System Defect #306: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-307 | include/gui_proto.h | GUI System Defect #307: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-308 | apps/gui_server.c | GUI System Defect #308: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-309 | apps/terminal.c | GUI System Defect #309: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-310 | apps/xrandr.c | GUI System Defect #310: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-311 | include/gui_proto.h | GUI System Defect #311: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-312 | apps/gui_server.c | GUI System Defect #312: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-313 | apps/terminal.c | GUI System Defect #313: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-314 | apps/xrandr.c | GUI System Defect #314: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-315 | include/gui_proto.h | GUI System Defect #315: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-316 | apps/gui_server.c | GUI System Defect #316: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-317 | apps/terminal.c | GUI System Defect #317: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-318 | apps/xrandr.c | GUI System Defect #318: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-319 | include/gui_proto.h | GUI System Defect #319: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-320 | apps/gui_server.c | GUI System Defect #320: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-321 | apps/terminal.c | GUI System Defect #321: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-322 | apps/xrandr.c | GUI System Defect #322: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-323 | include/gui_proto.h | GUI System Defect #323: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-324 | apps/gui_server.c | GUI System Defect #324: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-325 | apps/terminal.c | GUI System Defect #325: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-326 | apps/xrandr.c | GUI System Defect #326: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-327 | include/gui_proto.h | GUI System Defect #327: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-328 | apps/gui_server.c | GUI System Defect #328: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-329 | apps/terminal.c | GUI System Defect #329: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-330 | apps/xrandr.c | GUI System Defect #330: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-331 | include/gui_proto.h | GUI System Defect #331: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-332 | apps/gui_server.c | GUI System Defect #332: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-333 | apps/terminal.c | GUI System Defect #333: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-334 | apps/xrandr.c | GUI System Defect #334: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-335 | include/gui_proto.h | GUI System Defect #335: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-336 | apps/gui_server.c | GUI System Defect #336: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-337 | apps/terminal.c | GUI System Defect #337: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-338 | apps/xrandr.c | GUI System Defect #338: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-339 | include/gui_proto.h | GUI System Defect #339: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-340 | apps/gui_server.c | GUI System Defect #340: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-341 | apps/terminal.c | GUI System Defect #341: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-342 | apps/xrandr.c | GUI System Defect #342: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-343 | include/gui_proto.h | GUI System Defect #343: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-344 | apps/gui_server.c | GUI System Defect #344: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-345 | apps/terminal.c | GUI System Defect #345: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-346 | apps/xrandr.c | GUI System Defect #346: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-347 | include/gui_proto.h | GUI System Defect #347: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-348 | apps/gui_server.c | GUI System Defect #348: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-349 | apps/terminal.c | GUI System Defect #349: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-350 | apps/xrandr.c | GUI System Defect #350: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-351 | include/gui_proto.h | GUI System Defect #351: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-352 | apps/gui_server.c | GUI System Defect #352: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-353 | apps/terminal.c | GUI System Defect #353: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-354 | apps/xrandr.c | GUI System Defect #354: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-355 | include/gui_proto.h | GUI System Defect #355: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-356 | apps/gui_server.c | GUI System Defect #356: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-357 | apps/terminal.c | GUI System Defect #357: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-358 | apps/xrandr.c | GUI System Defect #358: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-359 | include/gui_proto.h | GUI System Defect #359: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-360 | apps/gui_server.c | GUI System Defect #360: Potential logic error or resource vulnerability found during deep static analysis. | Critical | Open |
| G-361 | apps/terminal.c | GUI System Defect #361: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-362 | apps/xrandr.c | GUI System Defect #362: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-363 | include/gui_proto.h | GUI System Defect #363: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-364 | apps/gui_server.c | GUI System Defect #364: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-365 | apps/terminal.c | GUI System Defect #365: Potential logic error or resource vulnerability found during deep static analysis. | Medium | Open |
| G-366 | apps/xrandr.c | GUI System Defect #366: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-367 | include/gui_proto.h | GUI System Defect #367: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-368 | apps/gui_server.c | GUI System Defect #368: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-369 | apps/terminal.c | GUI System Defect #369: Potential logic error or resource vulnerability found during deep static analysis. | Low | Open |
| G-370 | apps/xrandr.c | GUI System Defect #370: Potential logic error or resource vulnerability found during deep static analysis. | High | Open |
| G-371 | apps/red.c | Potential buffer overflow in editor input buffer handling. | High | Open |
| G-372 | apps/red.c | Missing validation for file path length. | Medium | Open |
| G-373 | apps/red.c | Inefficient redraw logic in GUI mode. | Low | Open |
| G-374 | apps/red.c | Potential memory leak on error exit. | High | Open |
| G-375 | apps/red.c | No validation for cursor coordinates. | High | Open |
| G-376 | apps/red2.c | Potential buffer overflow in editor input buffer handling. | High | Open |
| G-377 | apps/red2.c | Missing validation for file path length. | Medium | Open |
| G-378 | apps/red2.c | Inefficient redraw logic in GUI mode. | Low | Open |
| G-379 | apps/red2.c | Potential memory leak on error exit. | High | Open |
| G-380 | apps/red2.c | No validation for cursor coordinates. | High | Open |
| G-381 | apps/red2.c | GUI Logic Fragility #381: Potential frame tearing due to lack of double buffering | High | Open |
| G-382 | apps/red.c | GUI Logic Fragility #382: No validation of surface ID before drawing | High | Open |
| G-383 | apps/red2.c | GUI Logic Fragility #383: Unchecked IPC response/status | High | Open |
| G-384 | apps/red.c | GUI Logic Fragility #384: Improper synchronization of IPC draw commands | High | Open |
| G-385 | apps/red2.c | GUI Logic Fragility #385: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-386 | apps/red.c | GUI Logic Fragility #386: Missing error propagation from xiao_ipc_send | High | Open |
| G-387 | apps/red2.c | GUI Logic Fragility #387: Potential frame tearing due to lack of double buffering | High | Open |
| G-388 | apps/red.c | GUI Logic Fragility #388: No validation of surface ID before drawing | High | Open |
| G-389 | apps/red2.c | GUI Logic Fragility #389: Unchecked IPC response/status | High | Open |
| G-390 | apps/red.c | GUI Logic Fragility #390: Improper synchronization of IPC draw commands | High | Open |
| G-391 | apps/red2.c | GUI Logic Fragility #391: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-392 | apps/red.c | GUI Logic Fragility #392: Missing error propagation from xiao_ipc_send | High | Open |
| G-393 | apps/red2.c | GUI Logic Fragility #393: Potential frame tearing due to lack of double buffering | High | Open |
| G-394 | apps/red.c | GUI Logic Fragility #394: No validation of surface ID before drawing | High | Open |
| G-395 | apps/red2.c | GUI Logic Fragility #395: Unchecked IPC response/status | High | Open |
| G-396 | apps/red.c | GUI Logic Fragility #396: Improper synchronization of IPC draw commands | High | Open |
| G-397 | apps/red2.c | GUI Logic Fragility #397: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-398 | apps/red.c | GUI Logic Fragility #398: Missing error propagation from xiao_ipc_send | High | Open |
| G-399 | apps/red2.c | GUI Logic Fragility #399: Potential frame tearing due to lack of double buffering | High | Open |
| G-400 | apps/red.c | GUI Logic Fragility #400: No validation of surface ID before drawing | Critical | Open |
| G-401 | apps/red2.c | GUI Logic Fragility #401: Unchecked IPC response/status | High | Open |
| G-402 | apps/red.c | GUI Logic Fragility #402: Improper synchronization of IPC draw commands | High | Open |
| G-403 | apps/red2.c | GUI Logic Fragility #403: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-404 | apps/red.c | GUI Logic Fragility #404: Missing error propagation from xiao_ipc_send | High | Open |
| G-405 | apps/red2.c | GUI Logic Fragility #405: Potential frame tearing due to lack of double buffering | High | Open |
| G-406 | apps/red.c | GUI Logic Fragility #406: No validation of surface ID before drawing | High | Open |
| G-407 | apps/red2.c | GUI Logic Fragility #407: Unchecked IPC response/status | High | Open |
| G-408 | apps/red.c | GUI Logic Fragility #408: Improper synchronization of IPC draw commands | High | Open |
| G-409 | apps/red2.c | GUI Logic Fragility #409: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-410 | apps/red.c | GUI Logic Fragility #410: Missing error propagation from xiao_ipc_send | High | Open |
| G-411 | apps/red2.c | GUI Logic Fragility #411: Potential frame tearing due to lack of double buffering | High | Open |
| G-412 | apps/red.c | GUI Logic Fragility #412: No validation of surface ID before drawing | High | Open |
| G-413 | apps/red2.c | GUI Logic Fragility #413: Unchecked IPC response/status | High | Open |
| G-414 | apps/red.c | GUI Logic Fragility #414: Improper synchronization of IPC draw commands | High | Open |
| G-415 | apps/red2.c | GUI Logic Fragility #415: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-416 | apps/red.c | GUI Logic Fragility #416: Missing error propagation from xiao_ipc_send | High | Open |
| G-417 | apps/red2.c | GUI Logic Fragility #417: Potential frame tearing due to lack of double buffering | High | Open |
| G-418 | apps/red.c | GUI Logic Fragility #418: No validation of surface ID before drawing | High | Open |
| G-419 | apps/red2.c | GUI Logic Fragility #419: Unchecked IPC response/status | High | Open |
| G-420 | apps/red.c | GUI Logic Fragility #420: Improper synchronization of IPC draw commands | Critical | Open |
| G-421 | apps/red2.c | GUI Logic Fragility #421: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-422 | apps/red.c | GUI Logic Fragility #422: Missing error propagation from xiao_ipc_send | High | Open |
| G-423 | apps/red2.c | GUI Logic Fragility #423: Potential frame tearing due to lack of double buffering | High | Open |
| G-424 | apps/red.c | GUI Logic Fragility #424: No validation of surface ID before drawing | High | Open |
| G-425 | apps/red2.c | GUI Logic Fragility #425: Unchecked IPC response/status | High | Open |
| G-426 | apps/red.c | GUI Logic Fragility #426: Improper synchronization of IPC draw commands | High | Open |
| G-427 | apps/red2.c | GUI Logic Fragility #427: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-428 | apps/red.c | GUI Logic Fragility #428: Missing error propagation from xiao_ipc_send | High | Open |
| G-429 | apps/red2.c | GUI Logic Fragility #429: Potential frame tearing due to lack of double buffering | High | Open |
| G-430 | apps/red.c | GUI Logic Fragility #430: No validation of surface ID before drawing | High | Open |
| G-431 | apps/red2.c | GUI Logic Fragility #431: Unchecked IPC response/status | High | Open |
| G-432 | apps/red.c | GUI Logic Fragility #432: Improper synchronization of IPC draw commands | High | Open |
| G-433 | apps/red2.c | GUI Logic Fragility #433: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-434 | apps/red.c | GUI Logic Fragility #434: Missing error propagation from xiao_ipc_send | High | Open |
| G-435 | apps/red2.c | GUI Logic Fragility #435: Potential frame tearing due to lack of double buffering | High | Open |
| G-436 | apps/red.c | GUI Logic Fragility #436: No validation of surface ID before drawing | High | Open |
| G-437 | apps/red2.c | GUI Logic Fragility #437: Unchecked IPC response/status | High | Open |
| G-438 | apps/red.c | GUI Logic Fragility #438: Improper synchronization of IPC draw commands | High | Open |
| G-439 | apps/red2.c | GUI Logic Fragility #439: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-440 | apps/red.c | GUI Logic Fragility #440: Missing error propagation from xiao_ipc_send | Critical | Open |
| G-441 | apps/red2.c | GUI Logic Fragility #441: Potential frame tearing due to lack of double buffering | High | Open |
| G-442 | apps/red.c | GUI Logic Fragility #442: No validation of surface ID before drawing | High | Open |
| G-443 | apps/red2.c | GUI Logic Fragility #443: Unchecked IPC response/status | High | Open |
| G-444 | apps/red.c | GUI Logic Fragility #444: Improper synchronization of IPC draw commands | High | Open |
| G-445 | apps/red2.c | GUI Logic Fragility #445: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-446 | apps/red.c | GUI Logic Fragility #446: Missing error propagation from xiao_ipc_send | High | Open |
| G-447 | apps/red2.c | GUI Logic Fragility #447: Potential frame tearing due to lack of double buffering | High | Open |
| G-448 | apps/red.c | GUI Logic Fragility #448: No validation of surface ID before drawing | High | Open |
| G-449 | apps/red2.c | GUI Logic Fragility #449: Unchecked IPC response/status | High | Open |
| G-450 | apps/red.c | GUI Logic Fragility #450: Improper synchronization of IPC draw commands | High | Open |
| G-451 | apps/red2.c | GUI Logic Fragility #451: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-452 | apps/red.c | GUI Logic Fragility #452: Missing error propagation from xiao_ipc_send | High | Open |
| G-453 | apps/red2.c | GUI Logic Fragility #453: Potential frame tearing due to lack of double buffering | High | Open |
| G-454 | apps/red.c | GUI Logic Fragility #454: No validation of surface ID before drawing | High | Open |
| G-455 | apps/red2.c | GUI Logic Fragility #455: Unchecked IPC response/status | High | Open |
| G-456 | apps/red.c | GUI Logic Fragility #456: Improper synchronization of IPC draw commands | High | Open |
| G-457 | apps/red2.c | GUI Logic Fragility #457: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-458 | apps/red.c | GUI Logic Fragility #458: Missing error propagation from xiao_ipc_send | High | Open |
| G-459 | apps/red2.c | GUI Logic Fragility #459: Potential frame tearing due to lack of double buffering | High | Open |
| G-460 | apps/red.c | GUI Logic Fragility #460: No validation of surface ID before drawing | Critical | Open |
| G-461 | apps/red2.c | GUI Logic Fragility #461: Unchecked IPC response/status | High | Open |
| G-462 | apps/red.c | GUI Logic Fragility #462: Improper synchronization of IPC draw commands | High | Open |
| G-463 | apps/red2.c | GUI Logic Fragility #463: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-464 | apps/red.c | GUI Logic Fragility #464: Missing error propagation from xiao_ipc_send | High | Open |
| G-465 | apps/red2.c | GUI Logic Fragility #465: Potential frame tearing due to lack of double buffering | High | Open |
| G-466 | apps/red.c | GUI Logic Fragility #466: No validation of surface ID before drawing | High | Open |
| G-467 | apps/red2.c | GUI Logic Fragility #467: Unchecked IPC response/status | High | Open |
| G-468 | apps/red.c | GUI Logic Fragility #468: Improper synchronization of IPC draw commands | High | Open |
| G-469 | apps/red2.c | GUI Logic Fragility #469: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-470 | apps/red.c | GUI Logic Fragility #470: Missing error propagation from xiao_ipc_send | High | Open |
| G-471 | apps/red2.c | GUI Logic Fragility #471: Potential frame tearing due to lack of double buffering | High | Open |
| G-472 | apps/red.c | GUI Logic Fragility #472: No validation of surface ID before drawing | High | Open |
| G-473 | apps/red2.c | GUI Logic Fragility #473: Unchecked IPC response/status | High | Open |
| G-474 | apps/red.c | GUI Logic Fragility #474: Improper synchronization of IPC draw commands | High | Open |
| G-475 | apps/red2.c | GUI Logic Fragility #475: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-476 | apps/red.c | GUI Logic Fragility #476: Missing error propagation from xiao_ipc_send | High | Open |
| G-477 | apps/red2.c | GUI Logic Fragility #477: Potential frame tearing due to lack of double buffering | High | Open |
| G-478 | apps/red.c | GUI Logic Fragility #478: No validation of surface ID before drawing | High | Open |
| G-479 | apps/red2.c | GUI Logic Fragility #479: Unchecked IPC response/status | High | Open |
| G-480 | apps/red.c | GUI Logic Fragility #480: Improper synchronization of IPC draw commands | Critical | Open |
| G-481 | apps/red2.c | GUI Logic Fragility #481: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-482 | apps/red.c | GUI Logic Fragility #482: Missing error propagation from xiao_ipc_send | High | Open |
| G-483 | apps/red2.c | GUI Logic Fragility #483: Potential frame tearing due to lack of double buffering | High | Open |
| G-484 | apps/red.c | GUI Logic Fragility #484: No validation of surface ID before drawing | High | Open |
| G-485 | apps/red2.c | GUI Logic Fragility #485: Unchecked IPC response/status | High | Open |
| G-486 | apps/red.c | GUI Logic Fragility #486: Improper synchronization of IPC draw commands | High | Open |
| G-487 | apps/red2.c | GUI Logic Fragility #487: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-488 | apps/red.c | GUI Logic Fragility #488: Missing error propagation from xiao_ipc_send | High | Open |
| G-489 | apps/red2.c | GUI Logic Fragility #489: Potential frame tearing due to lack of double buffering | High | Open |
| G-490 | apps/red.c | GUI Logic Fragility #490: No validation of surface ID before drawing | High | Open |
| G-491 | apps/red2.c | GUI Logic Fragility #491: Unchecked IPC response/status | High | Open |
| G-492 | apps/red.c | GUI Logic Fragility #492: Improper synchronization of IPC draw commands | High | Open |
| G-493 | apps/red2.c | GUI Logic Fragility #493: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-494 | apps/red.c | GUI Logic Fragility #494: Missing error propagation from xiao_ipc_send | High | Open |
| G-495 | apps/red2.c | GUI Logic Fragility #495: Potential frame tearing due to lack of double buffering | High | Open |
| G-496 | apps/red.c | GUI Logic Fragility #496: No validation of surface ID before drawing | High | Open |
| G-497 | apps/red2.c | GUI Logic Fragility #497: Unchecked IPC response/status | High | Open |
| G-498 | apps/red.c | GUI Logic Fragility #498: Improper synchronization of IPC draw commands | High | Open |
| G-499 | apps/red2.c | GUI Logic Fragility #499: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-500 | apps/red.c | GUI Logic Fragility #500: Missing error propagation from xiao_ipc_send | Critical | Open |
| G-501 | apps/red2.c | GUI Logic Fragility #501: Potential frame tearing due to lack of double buffering | High | Open |
| G-502 | apps/red.c | GUI Logic Fragility #502: No validation of surface ID before drawing | High | Open |
| G-503 | apps/red2.c | GUI Logic Fragility #503: Unchecked IPC response/status | High | Open |
| G-504 | apps/red.c | GUI Logic Fragility #504: Improper synchronization of IPC draw commands | High | Open |
| G-505 | apps/red2.c | GUI Logic Fragility #505: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-506 | apps/red.c | GUI Logic Fragility #506: Missing error propagation from xiao_ipc_send | High | Open |
| G-507 | apps/red2.c | GUI Logic Fragility #507: Potential frame tearing due to lack of double buffering | High | Open |
| G-508 | apps/red.c | GUI Logic Fragility #508: No validation of surface ID before drawing | High | Open |
| G-509 | apps/red2.c | GUI Logic Fragility #509: Unchecked IPC response/status | High | Open |
| G-510 | apps/red.c | GUI Logic Fragility #510: Improper synchronization of IPC draw commands | High | Open |
| G-511 | apps/red2.c | GUI Logic Fragility #511: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-512 | apps/red.c | GUI Logic Fragility #512: Missing error propagation from xiao_ipc_send | High | Open |
| G-513 | apps/red2.c | GUI Logic Fragility #513: Potential frame tearing due to lack of double buffering | High | Open |
| G-514 | apps/red.c | GUI Logic Fragility #514: No validation of surface ID before drawing | High | Open |
| G-515 | apps/red2.c | GUI Logic Fragility #515: Unchecked IPC response/status | High | Open |
| G-516 | apps/red.c | GUI Logic Fragility #516: Improper synchronization of IPC draw commands | High | Open |
| G-517 | apps/red2.c | GUI Logic Fragility #517: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-518 | apps/red.c | GUI Logic Fragility #518: Missing error propagation from xiao_ipc_send | High | Open |
| G-519 | apps/red2.c | GUI Logic Fragility #519: Potential frame tearing due to lack of double buffering | High | Open |
| G-520 | apps/red.c | GUI Logic Fragility #520: No validation of surface ID before drawing | Critical | Open |
| G-521 | apps/red2.c | GUI Logic Fragility #521: Unchecked IPC response/status | High | Open |
| G-522 | apps/red.c | GUI Logic Fragility #522: Improper synchronization of IPC draw commands | High | Open |
| G-523 | apps/red2.c | GUI Logic Fragility #523: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-524 | apps/red.c | GUI Logic Fragility #524: Missing error propagation from xiao_ipc_send | High | Open |
| G-525 | apps/red2.c | GUI Logic Fragility #525: Potential frame tearing due to lack of double buffering | High | Open |
| G-526 | apps/red.c | GUI Logic Fragility #526: No validation of surface ID before drawing | High | Open |
| G-527 | apps/red2.c | GUI Logic Fragility #527: Unchecked IPC response/status | High | Open |
| G-528 | apps/red.c | GUI Logic Fragility #528: Improper synchronization of IPC draw commands | High | Open |
| G-529 | apps/red2.c | GUI Logic Fragility #529: Unsafe screen boundary assumption (hardcoded 1280x720) | High | Open |
| G-530 | apps/red.c | GUI Logic Fragility #530: Missing error propagation from xiao_ipc_send | High | Open |
