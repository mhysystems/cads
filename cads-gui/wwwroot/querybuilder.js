import * as THREE from 'https://cdn.skypack.dev/three@0.132.2';

const queryInput = "div .queryInput input";
var scene, camera, renderer;
var bX = 1600, bY = 4000, bZ = 100;

export function change() {
	const[dx,dy,dz] = [...document.querySelectorAll(queryInput)];

	const x = parseInt(dx.value);
	const y = parseInt(dy.value);
	const z = parseInt(dz.value);

	initMesh(scene, x, z*3, y)
}

export function update(x,y,z) {
  initMesh(scene, x, z*3, y)
}

function clearThree(obj) {
	while (obj.children.length > 0) {
		clearThree(obj.children[0])
		obj.remove(obj.children[0]);
	}
	if (obj.geometry) obj.geometry.dispose()

	if (obj.material) {
		//in case of map, bumpMap, normalMap, envMap ...
		Object.keys(obj.material).forEach(prop => {
			if (!obj.material[prop])
				return
			if (obj.material[prop] !== null && typeof obj.material[prop].dispose === 'function')
				obj.material[prop].dispose()
		})
		obj.material.dispose()
	}
}

function initMesh(sc, nx, ny, nz) {
	clearThree(scene)
	const geometry = new THREE.BoxGeometry(nx, ny, nz);
	let geo = new THREE.EdgesGeometry(geometry);
	let mat = new THREE.LineBasicMaterial({ color: 0x051d4b,  linewidth : 2 });
	let wireframe = new THREE.LineSegments(geo, mat);
	wireframe.position.set(0, (bZ - ny) / 2, (bY - nz) / 2)
	sc.add(wireframe);

	drawBelt(sc);
	drawLight(sc);
}

function drawBelt(sc) {

	const diffuseColor = new THREE.Color().setHSL(0, 0, 0.25);
	const materialPhysical = new THREE.MeshPhysicalMaterial({
		color: diffuseColor,
		//metalness: 0.5,
		roughness: 0.7,
		//clearcoat: 1.0,
		//clearcoatRoughness: 1.0,
		reflectivity: 1.0,
		transparent: true,
		opacity: 0.5,
		envMap: null,
		wireframe: false,
	});

	const beltGeo = new THREE.BoxGeometry(bX, bZ, bY);
	let cube = new THREE.Mesh(beltGeo, materialPhysical);
	sc.add(cube)
}

function drawLight(sc) {

	const light = new THREE.PointLight(0xffffff, 2);
	const light2 = new THREE.PointLight(0xffffff, 2);
	light.position.set(-1000, 1000, 1000);//.normalize();
	light2.position.set(-1000, -1000, 1000);//.normalize();

	sc.add(light, light2);
}

let mesh;

export function initThree(x,y,z) {
  bX = x;
  bY = y;
  bZ = 3 * z;
	scene = new THREE.Scene();
	const canvas = document.querySelector("#three");
	document.querySelector("div .X input").addEventListener("input", change);
	document.querySelector("div .Y input").addEventListener("input", change);
	document.querySelector("div .Z input").addEventListener("input", change);

	[...document.querySelectorAll(queryInput)].forEach(node => node.addEventListener("change", change));
	
	scene.background = new THREE.Color(0xffffff);
	// Create a basic perspective camera
	const { width, height } = canvas.getBoundingClientRect();
	camera = new THREE.PerspectiveCamera(75, width / height, 0.1, 10000);
	//camera = new THREE.OrthographicCamera( -2, 2, -2, 2, 0, 20 );
	camera.position.z = 2600;
	camera.position.x = 0; //-600;
	camera.position.y = 500;
	camera.lookAt(new THREE.Vector3(0, 0, 0));

	renderer = new THREE.WebGLRenderer({ antialias: true, canvas: canvas });
	renderer.setSize(width, height);
	renderer.setPixelRatio(window.devicePixelRatio);
	initMesh(scene, bX, bZ, bY)

	var render = function () {
		requestAnimationFrame(render);

		// Render the scene
		renderer.render(scene, camera);
	};

	render();
}
