var colours = [[0, 0, 0], [128, 0, 0], [0, 128, 0], [128, 128, 0], [0, 0, 128], [128, 0, 128], [0, 128, 128], [192, 192, 192], [128, 128, 128], [255, 0, 0], [0, 255, 0], [255, 255, 0], [0, 0, 255], [255, 0, 255], [0, 255, 255], [255, 255, 255], [0, 0, 0], [0, 0, 95], [0, 0, 135], [0, 0, 175], [0, 0, 215], [0, 0, 255], [0, 95, 0], [0, 95, 95], [0, 95, 135], [0, 95, 175], [0, 95, 215], [0, 95, 255], [0, 135, 0], [0, 135, 95], [0, 135, 135], [0, 135, 175], [0, 135, 215], [0, 135, 255], [0, 175, 0], [0, 175, 95], [0, 175, 135], [0, 175, 175], [0, 175, 215], [0, 175, 255], [0, 215, 0], [0, 215, 95], [0, 215, 135], [0, 215, 175], [0, 215, 215], [0, 215, 255], [0, 255, 0], [0, 255, 95], [0, 255, 135], [0, 255, 175], [0, 255, 215], [0, 255, 255], [95, 0, 0], [95, 0, 95], [95, 0, 135], [95, 0, 175], [95, 0, 215], [95, 0, 255], [95, 95, 0], [95, 95, 95], [95, 95, 135], [95, 95, 175], [95, 95, 215], [95, 95, 255], [95, 135, 0], [95, 135, 95], [95, 135, 135], [95, 135, 175], [95, 135, 215], [95, 135, 255], [95, 175, 0], [95, 175, 95], [95, 175, 135], [95, 175, 175], [95, 175, 215], [95, 175, 255], [95, 215, 0], [95, 215, 95], [95, 215, 135], [95, 215, 175], [95, 215, 215], [95, 215, 255], [95, 255, 0], [95, 255, 95], [95, 255, 135], [95, 255, 175], [95, 255, 215], [95, 255, 255], [135, 0, 0], [135, 0, 95], [135, 0, 135], [135, 0, 175], [135, 0, 215], [135, 0, 255], [135, 95, 0], [135, 95, 95], [135, 95, 135], [135, 95, 175], [135, 95, 215], [135, 95, 255], [135, 135, 0], [135, 135, 95], [135, 135, 135], [135, 135, 175], [135, 135, 215], [135, 135, 255], [135, 175, 0], [135, 175, 95], [135, 175, 135], [135, 175, 175], [135, 175, 215], [135, 175, 255], [135, 215, 0], [135, 215, 95], [135, 215, 135], [135, 215, 175], [135, 215, 215], [135, 215, 255], [135, 255, 0], [135, 255, 95], [135, 255, 135], [135, 255, 175], [135, 255, 215], [135, 255, 255], [175, 0, 0], [175, 0, 95], [175, 0, 135], [175, 0, 175], [175, 0, 215], [175, 0, 255], [175, 95, 0], [175, 95, 95], [175, 95, 135], [175, 95, 175], [175, 95, 215], [175, 95, 255], [175, 135, 0], [175, 135, 95], [175, 135, 135], [175, 135, 175], [175, 135, 215], [175, 135, 255], [175, 175, 0], [175, 175, 95], [175, 175, 135], [175, 175, 175], [175, 175, 215], [175, 175, 255], [175, 215, 0], [175, 215, 95], [175, 215, 135], [175, 215, 175], [175, 215, 215], [175, 215, 255], [175, 255, 0], [175, 255, 95], [175, 255, 135], [175, 255, 175], [175, 255, 215], [175, 255, 255], [215, 0, 0], [215, 0, 95], [215, 0, 135], [215, 0, 175], [215, 0, 215], [215, 0, 255], [215, 95, 0], [215, 95, 95], [215, 95, 135], [215, 95, 175], [215, 95, 215], [215, 95, 255], [215, 135, 0], [215, 135, 95], [215, 135, 135], [215, 135, 175], [215, 135, 215], [215, 135, 255], [223, 175, 0], [223, 175, 95], [223, 175, 135], [223, 175, 175], [223, 175, 223], [223, 175, 255], [223, 223, 0], [223, 223, 95], [223, 223, 135], [223, 223, 175], [223, 223, 223], [223, 223, 255], [223, 255, 0], [223, 255, 95], [223, 255, 135], [223, 255, 175], [223, 255, 223], [223, 255, 255], [255, 0, 0], [255, 0, 95], [255, 0, 135], [255, 0, 175], [255, 0, 223], [255, 0, 255], [255, 95, 0], [255, 95, 95], [255, 95, 135], [255, 95, 175], [255, 95, 223], [255, 95, 255], [255, 135, 0], [255, 135, 95], [255, 135, 135], [255, 135, 175], [255, 135, 223], [255, 135, 255], [255, 175, 0], [255, 175, 95], [255, 175, 135], [255, 175, 175], [255, 175, 223], [255, 175, 255], [255, 223, 0], [255, 223, 95], [255, 223, 135], [255, 223, 175], [255, 223, 223], [255, 223, 255], [255, 255, 0], [255, 255, 95], [255, 255, 135], [255, 255, 175], [255, 255, 223], [255, 255, 255], [8, 8, 8], [18, 18, 18], [28, 28, 28], [38, 38, 38], [48, 48, 48], [58, 58, 58], [68, 68, 68], [78, 78, 78], [88, 88, 88], [98, 98, 98], [108, 108, 108], [118, 118, 118], [128, 128, 128], [138, 138, 138], [148, 148, 148], [158, 158, 158], [168, 168, 168], [178, 178, 178], [188, 188, 188], [198, 198, 198], [208, 208, 208], [218, 218, 218], [228, 228, 228], [238, 238, 238]];

var canvas = document.getElementById("canvas");
var context = canvas.getContext("2d");
var image = context.getImageData(0, 0, canvas.width, canvas.height);
var pixels = image.data;

window.requestAnimationFrame = (window.requestAnimationFrame || window.mozRequestAnimationFrame || window.webkitRequestAnimationFrame || window.msRequestAnimationFrame);
canvas.requestFullscreen = (canvas.requestFullScreen || canvas.mozRequestFullScreen || canvas.webkitRequestFullscreen || canvas.msRequestFullscreen);

context.imageSmoothingEnabled = context.mozImageSmoothingEnabled = context.webkitImageSmoothingEnabled = context.msImageSmoothingEnabled = false;

var prevTime = Date.now();
var deltaTime;
var frameCounter = 0;

var frameSum = 0;
var prevFPS = 0;

function changeFrameBuffer () {
    image.data = pixels;
    context.putImageData(image, 0, 0);
}

function drawFrame () { // (Math.round((w % 20) / 20) ^ Math.round((h % 20) / 20)) * 255
    var c;
    
    for (var w = 0; w < canvas.width; ++w) {
        for (var h = 0; h < canvas.height; h++) {
            c = Math.round(Math.random() * 255);//Math.round(Number("0."+Math.sin((Math.floor(w / 20) + Math.floor(h / 20) * (canvas.width / 20)) + prevTime).toString().substr(6)) * 255);
            pixels[(h * canvas.width + w) * 4 + 0] = colours[c][0];
            pixels[(h * canvas.width + w) * 4 + 1] = colours[c][1];
            pixels[(h * canvas.width + w) * 4 + 2] = colours[c][2];
            pixels[(h * canvas.width + w) * 4 + 3] = 255;
        }
    }
}

function drawFrameRate () {
    frameSum += deltaTime;
    
    if ((frameCounter % 5) == 0) {
        prevFPS = Math.round(5 / frameSum);
        frameSum = 0;
    }
    
    context.font = "30px Arial";
    context.fillStyle = 'red';
    context.fillText(prevFPS, canvas.width - 32, canvas.height);
}

function update () {
    deltaTime = (Date.now() - prevTime) / 1000;
    prevTime = Date.now();
    
    frameCounter++;
    
    drawFrame();
    changeFrameBuffer();
    
    drawFrameRate();
    
    //window.requestAnimationFrame(update);
}

window.requestAnimationFrame(update);

// Voxel: 15 Bit first child - 1 Bit far pointer - 8 Bit children mask - 8 Bit colour === leaf: empty children mask, invalid: empty first child mask + empty far pointer mask
// Parent: 15 Bit 0s - 1 Bit 1 - 15 Bit parent - 1 Bit far pointer

// Pointer: 32 Bit pointer

var voxBuf = new ArrayBuffer(8);
var voxels = new Uint32Array(voxBuf);

voxels[0] = 0xFFFF0000;
voxels[1] = 0x00010000;

var poiBuf = new ArrayBuffer(4);
var points = new Uint32Array(poiBuf);

points[0] = 0x00000000;

var voxSpo = [];
var poiSpo = [];

function requestVoxelsSpace (amount) {
    if (!Number.isInteger(amount) || amount < 1) {
        amount = 1;
    }
    
    if (voxSpo.length >= amount) {
        return voxSpo.pop();
    }
    
    var bufTmp = new ArrayBuffer(voxBuf.byteLength + 4 * 9 * amount);
    var vieTmp = new Uint32Array(bufTmp);
    
    vieTmp.set(voxels);
    
    voxBuf = bufTmp;
    voxels = vieTmp;
    
    for (var i = 1; i < amount; i++) {
        voxSpo.push(voxBuf.byteLength - 4 * 9 * i);
    }
    
    return ((voxBuf.byteLength - 4 * 9 * amount) / 4);
}

function releaseVoxelsSpace (index) {
    if (!Number.isInteger(index) || index < 2) {
        return false;
    }
    
    for (var i = index; i < (index + 9); i++) {
        voxels[i] = 0x00000000;
    }
    
    if ((voxBuf.byteLength - (index * 4)) < 4 * 9) {
        return false;
    }
    else {
        voxSpo.push(index);
        
        return true;
    }
}

function requestPointsSpace (amount) {
    if (!Number.isInteger(amount) || amount < 1) {
        amount = 1;
    }
    
    if (poiSpo.length >= amount) {
        return poiSpo.pop();
    }
    
    var bufTmp = new ArrayBuffer(poiBuf.byteLength + 4 * amount);
    var vieTmp = new Uint32Array(bufTmp);
    
    vieTmp.set(points);
    
    poiBuf = bufTmp;
    points = vieTmp;
    
    for (var i = 1; i < amount; i++) {
        poiSpo.push(poiBuf.byteLength - 4 * i);
    }
    
    return ((poiBuf.byteLength - 4 * amount) / 4);
}

function releasePointsSpace (index) {
    if (!Number.isInteger(index) || index < 1) {
        return false;
    }
    
    points[i] = 0x00000000;
    
    if ((poiBuf.byteLength - (index * 4)) < 4) {
        return false;
    }
    else {
        poiSpo.push(index);
        
        return true;
    }
}

function setVoxel (parent, position, voxel) {
    if ((voxel >>> 16) == 1 || !Number.isInteger(position) || position < 0 || position > 7) {
        return false;
    }
    
    var Parent = voxels[parent];
    
    if (!Parent || (Parent >>> 17) == 0) {
        return false;
    }
    
    var FirstChild, Pointer;
    
    if (((Parent & 0x0000FF00) >>> 8) == 0) {
        if ((voxel >>> 16) == 0) {
            return true;
        }
        
        Parent = (Parent | (Math.pow(2, 7 - position) << 8));
        
        FirstChild = requestVoxelsSpace(1);
        
        if ((FirstChild & 0xFFFF8000) > 0) {
            Pointer = requestPointsSpace();
            points[Pointer] = FirstChild;
            
            Parent = ((Parent & 0x0000FFFF) | (Pointer << 17) | 0x00010000);
        }
        else {
            Parent = ((Parent & 0x0000FFFF) | (FirstChild << 17));
        }
        
        voxels[parent] = Parent;
        voxels[FirstChild] = voxels[FirstChild + 1] = voxels[FirstChild + 2] = voxels[FirstChild + 3] = voxels[FirstChild + 4] = voxels[FirstChild + 5] = voxels[FirstChild + 6] = voxels[FirstChild + 7] = 0x00000000;
        
        if ((parent & 0xFFFF8000) > 0) {
            Pointer = requestPointsSpace(1);
            points[Pointer] = parent;
            
            voxels[FirstChild + 8] = (0x00010001 | (Pointer << 1));
        }
        else {
            voxels[FirstChild + 8] = (0x00010000 | (parent << 1));
        }
    }
    else {
        FirstChild = ((Parent & 0x00010000) == 0) ? (Parent >>> 17) : (points[Parent >>> 17]);
        
        if ((((Parent & 0x0000FF00) >>> 8) & Math.pow(2, 7 - position)) > 0) {
            if (voxels[FirstChild + position] == voxel) {
                return true;
            }
            
            if ((voxel >>> 17) > 0) {
                delVoxel(FirstChild + position, false);
            }
            else {
                return delVoxel(FirstChild + position, true);
            }
            
            if (((voxel & 0x0000FF00) >>> 8) > 0) {
                if ((voxels[(((voxel & 0x00010000) == 0) ? (voxel >>> 17) : (points[voxel >>> 17])) + 8] & 0x00000001) == 1) {
                    releasePointsSpace(voxel >>> 17);
                }
                
                if ((voxel & 0xFFFF8000) > 0) {
                    Pointer = requestPointsSpace(1);
                    points[Pointer] = FirstChild + position;

                    voxels[(((voxel & 0x00010000) == 0) ? (voxel >>> 17) : (points[voxel >>> 17])) + 8] = (0x00010001 | (Pointer << 1));
                }
                else {
                    voxels[(((voxel & 0x00010000) == 0) ? (voxel >>> 17) : (points[voxel >>> 17])) + 8] = (0x00010000 | ((FirstChild + 8) << 1));
                }
            }
        }
        else if ((voxel >>> 17) == 0) {
            return true;
        }
        
        Parent = (Parent | (Math.pow(2, 7 - position) << 8));
        
        voxels[parent] = Parent;
    }
    
    voxels[FirstChild + position] = voxel;
    
    return true;
}

function delVoxel (index) {
    return delVoxel(index, true);
}

function delVoxel (index, trace) {
    
    if (!Number.isInteger(index) || index < 0) {
        return false;
    }
    
    var Voxel = voxels[index];
    
    if (!Voxel || (Voxel >>> 16) == 1) {
        return false;
    }
    
    if ((Voxel >>> 16) == 0) {
        return true;
    }
    
    if (((Voxel & 0x0000FF00) >>> 8) > 0) {
        var Child = (((Voxel & 0x00010000) == 0) ? (Voxel >>> 17) : (points[Voxel >>> 17]));
        
        if ((Voxel & 0x00010000) == 1) {
            releasePointsSpace(Voxel >>> 17);
        }
        
        for (var i = Child; i < (Child + 8); i++) {
            delVoxel(Child + i);
        }
        
        if ((voxels[Child + 8] & 0x00000001) == 1) {
            releasePointsSpace((voxels[Child + 8] & 0x0000FFFE) >>> 1);
        }
        
        releaseVoxelsSpace(Child);
    }
    
    if (index > 1) {
        voxels[index] = 0x00000000;
    
        var i = 1;
    
        while ((voxels[index + i] >>> 16) != 1) {
            i++;
        }
        
        var parent = (((voxels[index + i] & 0x00000001) == 0) ? ((voxels[index + i] & 0x0000FFFE) >>> 1) : (points[(voxels[index + i] & 0x0000FFFE) >>> 1]));
    
        if (trace && (((voxels[parent] & (~(Math.pow(2, i - 1) << 8))) & 0x0000FF00) >>> 8) == 0) {
            delVoxel(parent);
        }
        else {
            voxels[parent] = (voxels[parent] & (~(Math.pow(2, i - 1) << 8)));
        }
    }
    else {
        voxels[index] = ((Voxel & 0x000000FF) | 0xFFFF0000);
    }
    
    return true;
}

console.log(voxels);

setVoxel(0, 1, 0xFFFF0088);

console.log(voxels);

setVoxel(0, 1, 0x00000000);

console.log(voxels);

setVoxel(0, 2, 0xFFFF0089);

console.log(voxels);

/*function Voxel (v) {
    this.firstChild = v >>> 17;
    this.farPointer = (v & 65536) >>> 16;
    this.children = (v & 65280) >>> 8;
    this.size = ((this.children & 128) >>> 7) + ((this.children & 64) >>> 6) + ((this.children & 32) >>> 5) + ((this.children & 16) >>> 4) + ((this.children & 8) >>> 3) + ((this.children & 4) >>> 2) + ((this.children & 2) >>> 1) + (this.children & 1);
    this.colour = v & 255;
}

function compressVoxel (firstChild, farPointer, children, colour) {
    return (firstChild << 17) + (farPointer << 16) + (children << 8) + (colour);
}

function depthFirst (vs, i) {
    var v = new Voxel(vs[i]);
    
    for (var i = 0; i < v.size; ++i) {
        depthFirst(vs, (v.farPointer ? vs[v.firstChild] : v.firstChild) + i);
    }
    
    console.log(v);
}

voxels[0] = compressVoxel(1, false, 128|64|32|16|8|4|2|1, 255);
voxels[1] = compressVoxel(9, true, 64, 255);
voxels[2] = compressVoxel(0, false, 0, 255);
voxels[3] = compressVoxel(0, false, 0, 255);
voxels[4] = compressVoxel(0, false, 0, 255);
voxels[5] = compressVoxel(0, false, 0, 255);
voxels[6] = compressVoxel(0, false, 0, 255);
voxels[7] = compressVoxel(0, false, 0, 255);
voxels[8] = compressVoxel(0, false, 0, 255);
voxels[9] = 10;
voxels[10] = compressVoxel(0, false, 0, 255);

depthFirst(voxels, 0);*/