import sys

with open('pieces/apps/gl_os/plugins/gl_desktop.c', 'r') as f:
    content = f.read()

# Add the draw_voxel_piece function implementation right after draw_tile
draw_voxel_func_impl = """
void draw_voxel_piece(float x, float y, float z, float size, unsigned char dna[8][8], float r, float g, float b, int wireframe) {
    float v_size = size / 8.0f;
    float offset = -size / 2.0f + v_size / 2.0f;

    for (int iz = 0; iz < 8; iz++) {
        for (int iy = 0; iy < 8; iy++) {
            unsigned char row = dna[iz][iy];
            for (int ix = 0; ix < 8; ix++) {
                if (row & (1 << (7 - ix))) {
                    float vx = x + offset + (ix * v_size);
                    float vy = y + (iz * v_size);
                    float vz = z + offset + (iy * v_size);
                    
                    glPushMatrix();
                    glTranslatef(vx, vy, vz);
                    if (wireframe) {
                        glColor3f(r, g, b);
                        glutWireCube(v_size);
                    } else {
                        draw_tile(0, 0, 0, v_size, r, g, b);
                    }
                    glPopMatrix();
                }
            }
        }
    }
}
"""

# Find the end of draw_tile to insert the new function
# The function ends with '}'
insert_point = content.find('void draw_tile(float x, float y, float z, float size, float r, float g, float b) {')
if insert_point != -1:
    # Find the closing brace of draw_tile
    brace_count = 0
    end_idx = -1
    for i in range(len(content)):
        if content[i] == '{':
            brace_count += 1
        elif content[i] == '}':
            brace_count -= 1
            if brace_count == 0:
                end_idx = i + 1
                break
    
    if end_idx != -1:
        # Insert draw_voxel_piece right after the closing brace of draw_tile
        content = content[:end_idx] + '\n' + voxel_func_impl + '\n' + content[end_idx:]

with open('pieces/apps/gl_os/plugins/gl_desktop.c', 'w') as f:
    f.write(content)

# Compile to verify
print("Attempting to compile after adding draw_voxel_piece...")
