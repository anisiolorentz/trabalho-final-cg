#version 330 core





in vec4 position_world;
in vec4 normal;


in vec4 position_model;


in vec2 texcoords;


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;








#define SPHERE         0
#define BUNNY          1
#define PLANE          2
#define FLOOR          3
#define WALL           4
#define TABLE          5
#define CUBE_PIECE     6
#define TRIANGLE_PIECE 7
#define CYLINDER_PIECE 8
#define SELECTED_PIECE 9
#define CEILING        10
#define LIGHT_PANEL    11
#define SHADOW         12
#define EXIT_MARKER    14
#define BEZIER_TRAIL   15
#define DIRECTION_ARROW 16
#define CASTLE_DOOR   17
#define BALCONY_CONCRETE 18
uniform int object_id;


uniform vec4 bbox_min;
uniform vec4 bbox_max;







uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureImage5;
uniform float shadow_alpha;
uniform vec3 guide_color;


out vec4 color;


#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

float EdgeDarkening(float width)
{
    vec3 extent = max(bbox_max.xyz - bbox_min.xyz, vec3(0.001));
    vec3 q = clamp((position_model.xyz - bbox_min.xyz) / extent, 0.0, 1.0);
    float edge_distance = min(min(q.x, 1.0 - q.x), min(min(q.y, 1.0 - q.y), min(q.z, 1.0 - q.z)));
    float edge = 1.0 - smoothstep(0.0, width, edge_distance);
    return mix(1.0, 0.48, edge);
}

void main()
{
    
    
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    
    
    
    
    
    vec4 p = position_world;

    
    
    vec4 n = normalize(normal);

    
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    
    vec4 v = normalize(camera_position - p);

    
    float U = 0.0;
    float V = 0.0;

	
	vec3 Kd0;

    if ( object_id == SPHERE )
    {
        
        
        
        
        

        
        
        
        
        
        

        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;
        vec4 d = position_model - bbox_center;

        float rho   = length(d);
        float theta = atan(d.x,d.z);
        float phi   = asin(d.y / rho);

        U = (theta + M_PI) / 2.0 / M_PI;
        V = (phi + M_PI_2) / M_PI;

		
		Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    }
    else if ( object_id == BUNNY )
    {
        
        
        
        
        
        
        
        

        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x - minx) / (maxx - minx);
        V = (position_model.y - miny) / (maxy - miny);

		
		Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    }
    else if ( object_id == PLANE )
    {
        
        U = texcoords.x;
        V = texcoords.y;

		
		Kd0 = texture(TextureImage1, vec2(U,V)).rgb;
    }
    else if ( object_id == FLOOR )
    {
        
        
        
        
        
        
        U = fract(position_world.x / 2.0);
        V = fract(position_world.z / 2.0);
        Kd0 = texture(TextureImage2, vec2(U,V)).rgb; 
    }
    else if ( object_id == WALL )
    {
        
        
        
        
        
        
        float u_raw, v_raw;
        if (abs(n.x) > abs(n.z))
        {
            u_raw = position_world.z / 2.0;
            v_raw = position_world.y / 2.0;
        }
        else
        {
            u_raw = position_world.x / 2.0;
            v_raw = position_world.y / 2.0;
        }
        U = fract(u_raw);
        V = fract(v_raw);
        Kd0 = texture(TextureImage0, vec2(U,V)).rgb; 
    }
    else if ( object_id == CEILING )
    {
        U = fract(position_world.x / 2.5);
        V = fract(position_world.z / 2.5);
        vec3 subtle = texture(TextureImage2, vec2(U,V)).rgb;
        Kd0 = mix(vec3(0.62, 0.63, 0.58), subtle, 0.12);
    }
    else if ( object_id == LIGHT_PANEL )
    {
        color = vec4(1.0, 0.98, 0.88, 1.0);
        return;
    }
    else if ( object_id == SHADOW )
    {
        color = vec4(0.0, 0.0, 0.0, shadow_alpha);
        return;
    }
    else if ( object_id == EXIT_MARKER )
    {
        color = vec4(0.42, 0.95, 0.82, 1.0);
        return;
    }
    else if ( object_id == BEZIER_TRAIL || object_id == DIRECTION_ARROW )
    {
        color = vec4(guide_color, 1.0);
        return;
    }
    else if ( object_id == BALCONY_CONCRETE )
    {
        float tile_scale = 0.95;
        if ( abs(normal.y) > 0.5 )
        {
            U = fract(position_world.x / tile_scale);
            V = fract(position_world.z / tile_scale);
        }
        else if ( abs(normal.x) > 0.5 )
        {
            U = fract(position_world.z / tile_scale);
            V = fract(position_world.y / tile_scale);
        }
        else
        {
            U = fract(position_world.x / tile_scale);
            V = fract(position_world.y / tile_scale);
        }
        Kd0 = texture(TextureImage5, vec2(U,V)).rgb;
    }
    else if ( object_id == CASTLE_DOOR )
    {
        U = texcoords.x;
        V = texcoords.y;
        Kd0 = texture(TextureImage4, vec2(U,V)).rgb;
    }
    else if ( object_id == TABLE )
    {
        
        
        
        U = texcoords.x;
        V = texcoords.y;
        Kd0 = texture(TextureImage3, vec2(U,V)).rgb;
    }
    else if ( object_id == CUBE_PIECE )
    {
        
        U = position_model.x * 1.5;
        V = position_model.z * 1.5 + position_model.y;

        vec3 base = texture(TextureImage0, vec2(U,V)).rgb;
        Kd0 = vec3(0.10, 0.62, 0.20) * EdgeDarkening(0.09);
    }
    else if ( object_id == TRIANGLE_PIECE )
    {
        
        U = position_model.x * 1.5;
        V = position_model.z * 1.5 + position_model.y;

        vec3 base = texture(TextureImage0, vec2(U,V)).rgb;
        Kd0 = vec3(0.78, 0.09, 0.06) * EdgeDarkening(0.10);
    }
    else if ( object_id == CYLINDER_PIECE )
    {
        
        U = position_model.x * 1.5;
        V = position_model.z * 1.5 + position_model.y;

        vec3 base = texture(TextureImage0, vec2(U,V)).rgb;
        Kd0 = vec3(0.90, 0.66, 0.04) * EdgeDarkening(0.12);
    }
    else if ( object_id == SELECTED_PIECE )
    {
        
        U = position_model.x * 1.5;
        V = position_model.z * 1.5 + position_model.y;

        vec3 base = texture(TextureImage0, vec2(U,V)).rgb;
        Kd0 = vec3(0.80, 0.80, 0.80) * EdgeDarkening(0.10);
    }
    else
    {
        
        Kd0 = vec3(1.0, 0.0, 1.0);
    }

    
    vec3 p3 = p.xyz;
    vec3 n3 = normalize(n.xyz);
    vec3 v3 = normalize(v.xyz);

    vec3 light0 = vec3(-6.5, 13.4, -3.8);
    vec3 light1 = vec3( 0.0, 13.4,  0.0);
    vec3 light2 = vec3( 6.5, 13.4,  3.8);

    vec3 l0 = normalize(light0 - p3);
    vec3 l1 = normalize(light1 - p3);
    vec3 l2 = normalize(light2 - p3);

    float diffuse = 0.34 * max(0.0, dot(n3, l0))
                  + 0.42 * max(0.0, dot(n3, l1))
                  + 0.34 * max(0.0, dot(n3, l2));

    vec3 h0 = normalize(l0 + v3);
    vec3 h1 = normalize(l1 + v3);
    vec3 h2 = normalize(l2 + v3);
    float specular = pow(max(0.0, dot(n3, h0)), 24.0) * 0.10
                   + pow(max(0.0, dot(n3, h1)), 24.0) * 0.14
                   + pow(max(0.0, dot(n3, h2)), 24.0) * 0.10;

    float ambient = 0.38;
    color.rgb = Kd0 * (ambient + diffuse) + vec3(specular);

    
    
    
    
    
    
    
    
    
    
    
    
    color.a = 1;

    
    
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
} 





