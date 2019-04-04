#version 450
uniform sampler2D wave;
uniform int donttouch;
out vec4 fragCol;

float maxdist = 100.0;

uint rand = 0u;
void stepState()
{
	rand = rand ^ (rand << 13u);
	rand = rand ^ (rand >> 17u);
	rand = rand ^ (rand << 5u);
	rand *= 1685821657u;
}

void feed(float value)
{
	rand += floatBitsToUint(value);
	stepState();
}

float getFloat() {
	stepState();
	return uintBitsToFloat( (rand & 0x007FFFFFu) | 0x3F800000u ) - 1.5;
}

vec3 getVec3() {
	return vec3(getFloat(),getFloat(),getFloat());
}

struct Ray
{
	vec3 m_origin;
	vec3 m_direction;
	vec3 m_point;
	bool m_intersected;
	vec3 m_color;
	vec3 m_attenuation;
};

Ray newRay(vec3 origin, vec3 direction, vec3 attenuation) {
		// Create a default ray
		return Ray(origin, direction, origin, false, vec3(0.0), attenuation);
}

float heightmap(vec2 uv) {
	return texture2D(wave, uv*0.1).x*0.8;
}

vec3 heightmapNormal(vec2 uv) {
	vec2 epsi = vec2(0.001, 0.0);
	float xdiff = heightmap(uv) - heightmap(uv+epsi.xy);
	float ydiff = heightmap(uv) - heightmap(uv+epsi.yx);
	return normalize(cross(vec3(epsi.yx, -xdiff), vec3(epsi.xy, -ydiff)));
}

void castRay(inout Ray ray) {
	// Cast ray from origin into scene
	float dt = 0.15;
	float lastdiff = 0.0;
	for (int i = 0; i < 100; i++) {
		if (distance(ray.m_origin, ray.m_point) > maxdist) return;
		float height = heightmap(ray.m_point.xy);
		float diff = ray.m_point.z - height;

		if (diff < 0.0) {
			ray.m_point += dt * diff / (lastdiff - diff) * ray.m_direction;
			ray.m_intersected = true;
			return;
		}

		dt = dt*1.01;
		ray.m_point += dt * ray.m_direction;
		lastdiff = diff;
	}
}

vec3 skyDomeShade(vec3 angle) {
	float horizon = pow(abs(angle.z), 0.5);
	return mix(vec3(1.2, 1.0, 0.75),vec3(0.4,0.75,1.0), horizon) + pow(max(dot(angle, normalize(vec3(-1.0,-1.0,0.1))),0.0),1000.0)*4.0;
}

void phongShadeRay(inout Ray ray) {
	if (!ray.m_intersected) {
		ray.m_color = skyDomeShade(ray.m_direction);
	} else {
	float fading = pow(max(maxdist - distance(ray.m_origin, ray.m_point), 0.0)/maxdist, 2.0);
		ray.m_color = (1.0-fading)*skyDomeShade(ray.m_direction);
	}
}

Ray reflectionForRay(Ray ray) {
	float fading = pow(max(maxdist - distance(ray.m_origin, ray.m_point), 0.0)/maxdist, 2.0);
	vec3 normal = -heightmapNormal(ray.m_point.xy);
	float frensel = abs(dot(ray.m_direction, normal));
	vec3 atten = (fading) * ray.m_attenuation * 0.9 * (1.0 - frensel*0.98);
	vec3 reflected = reflect(ray.m_direction, normal);

	return newRay(ray.m_point + normal*0.1, reflected, atten);
}

Ray rayQueue[MAXDEPTH];
int raynum = 1;
void addToQueue(Ray ray) {
		if (raynum >= MAXDEPTH) return;
		rayQueue[raynum] = ray;
		raynum++;
}

void recursivelyRender(inout Ray ray) {
	ray.m_point += ray.m_direction*4.0;
		rayQueue[0] = ray;

		for (int i = 0; i < MAXDEPTH; i++) {
				if (i >= raynum) break;

				castRay(rayQueue[i]);
				phongShadeRay(rayQueue[i]);
				if (rayQueue[i].m_intersected) {
						addToQueue(reflectionForRay(rayQueue[i]));
				// 		// addToQueue(transmissionForRay(rayQueue[i]));
				}
		}
		for (int i = 0; i < raynum; i++) {
				ray.m_color += rayQueue[i].m_color * rayQueue[i].m_attenuation;
		}
		raynum = 1;
}

void main() {
		// Normalized pixel coordinates (from -1 to 1)
		vec2 uv = (gl_FragCoord.xy - vec2(960.0, 540.0))/vec2(960.0, 960.0);

		feed(uv.x);
		feed(uv.y);

		// Camera parameters

		vec3 col = vec3(0.0);

		int maxsamples = 10 + donttouch;
		for (int i = 0; i < maxsamples; i++) {
			vec3 cameraOrigin = vec3(5.0, 5.0, heightmap(vec2(5.0, 5.0))+2.5) + normalize(getVec3())*0.05;
			vec3 focusOrigin = vec3(0.0, 0.0, heightmap(vec2(0.0)));
			vec3 cameraDirection = normalize(focusOrigin-cameraOrigin);

			vec3 up = vec3(0.0,0.0,-1.0);
			vec3 plateXAxis = normalize(cross(cameraDirection, up));
			vec3 plateYAxis = normalize(cross(cameraDirection, plateXAxis));

			float fov = radians(50.0);

			vec3 platePoint = (plateXAxis * -uv.x + plateYAxis * uv.y) * tan(fov /2.0);

			Ray ray = newRay(cameraOrigin, normalize(platePoint + cameraDirection), vec3(1.0));
			recursivelyRender(ray);
			col += ray.m_color;//*(1.0 - pow(length(uv)*0.85, 3.0));
		}
		col /= float(maxsamples);
		col *= (1.0 - pow(length(uv)*0.8, 2.0)); //vingetting lol
		fragCol = vec4(pow(log(col+1.0), vec3(1.3)), 1.0);

		// fragCol = (texture2D(wave, uv).xxxx+1.0)/2.0;
}