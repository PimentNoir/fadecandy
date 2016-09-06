
boolean[] keys;

void keyPressed() {
  if (key == '0') {
    keys[0] = true;
  }
  if (key == '1') {
    keys[1] = true;
  }
  if (key == '2') {
    keys[2] = true;
  }
  if (key == '3') {
    keys[3] = true;
  }
  if (key == '4') {
    keys[4] = true;
  }
  if (key == '5') {
    keys[5] = true;
  }
  if (key == '6') {
    keys[6] = true;
  }
  if (key == '7') {
    keys[7] = true;
  }
  if (key == '8') {
    keys[8] = true;
  }
  if (key == '9') {
    keys[9] = true;
  }
  if (key == '-') {
    keys[10] = true;
  }
  if (key == '+') {
    keys[11] = true;
  }
  if (key == 't') {
    keys[12] = true;
  }
  if (key == 'n') {
    keys[13] = true;
  }
  if (key == 'd') {
    keys[14] = true;
  }
  if (key == 'p') {
    keys[15] = true;
  }
  if (key == 'l') {
    keys[16] = true;
  }
  if (key == 'b') {
    keys[17] = true;
  }
  if (key == 'a') {
    keys[18] = true;
  }
  if (key == 's') {
    keys[19] = true;
  }
  if (key == 'q') {
    keys[20] = true;
  }
  if (key == 'h') {
    keys[21] = true;
  }
  if (key == 'e') {
    keys[22] = true;
  }
  if (key == 'w') {
    keys[23] = true;
  }
  if (key == 'r') {
    keys[24] = true;
  }
  if (key == 'c') {
    keys[25] = true;
  }
  if (key == 'i') {
    keys[26] = true;
  }
  if (key == 'z') {
    keys[27] = true;
  }
  if (key == 'm') {
    keys[28] = true;
  }
  if (key == 'o') {
    keys[29] = true;
  }
  // Count number of keys pressed
  int nrKeys = 0;
  for (int i = 0; i < keys.length; i++) {
    if (keys[i]) {
      nrKeys++;
    }
  }
  float noise_scale_inc = 0.001f;
  if (keys[13] && keys[11] && noise_scale_fft < 15-noise_scale_inc) {
    noise_scale_fft += noise_scale_inc;
    Debug.UndoPrinting();
  }
  if (keys[13] && keys[10] && noise_scale_fft > noise_scale_inc) {
    noise_scale_fft -= noise_scale_inc;
    Debug.UndoPrinting();
  }
  float inc = 0.01f;
  if (keys[14] && keys[11] && decay < 1-inc && !useEMA) {
    decay += inc;
    Debug.UndoPrinting();
  }
  if (keys[14] && keys[10] && decay > inc && !useEMA) {
    decay -= inc;
    Debug.UndoPrinting();
  }
  if (keys[15] && keys[11]) {
    persistence += inc;
    Debug.UndoPrinting();
  }
  if (keys[15] && keys[10] && persistence > inc) {
    persistence -= inc;
    Debug.UndoPrinting();
  }
  if (keys[16] && keys[11]) {
    lacunarity += inc;
    Debug.UndoPrinting();
  }
  if (keys[16] && keys[10] && lacunarity > inc) {
    lacunarity -= inc;
    Debug.UndoPrinting();
  }
  float beat_inc = 0.5f;
  if (keys[17] && keys[11] && beat_ratio < 64) {
    beat_ratio += beat_inc;
    Debug.UndoPrinting();
  }
  if (keys[17] && keys[10] && beat_ratio > 0) {
    beat_ratio -= beat_inc;
    Debug.UndoPrinting();
  }
  int phi_inc = 1;
  if (keys[18] && keys[11] && phideg < 360) {
    phideg += phi_inc;
    Debug.UndoPrinting();
  }
  if (keys[18] && keys[10] && phideg > 0) {
    phideg -= phi_inc;
    Debug.UndoPrinting();
  }
  if (keys[19] && keys[11] && smooth_factor < 1-inc && useEMA) {
    smooth_factor += inc;
    Debug.UndoPrinting();
  }
  if (keys[19] && keys[10] && smooth_factor > inc && useEMA) { 
    smooth_factor -= inc;
    Debug.UndoPrinting();
  }
  if (keys[20] && keys[11] && minSize < 1-inc) { 
    minSize += inc;
    Debug.UndoPrinting();
  }
  if (keys[20] && keys[10] && minSize > inc) { 
    minSize -= inc;
    Debug.UndoPrinting();
  }
  if (keys[21] && keys[11] && sizeScale < 1-inc) { 
    sizeScale += inc;
    Debug.UndoPrinting();
  }
  if (keys[21] && keys[10] && sizeScale > inc) { 
    sizeScale -= inc;
    Debug.UndoPrinting();
  }
  if (keys[22] && keys[12]) {
    useEMA = !useEMA;
    Debug.UndoPrinting();
  }
  if (keys[23] && keys[12] && isPlayer) {
    isWebPlayer = !isWebPlayer;
    Debug.UndoPrinting();
  }
  if (key == 'g') {
    Debug.UndoPrinting();
  }
  if (keys[25] && keys[12]) {
    isColorFile = !isColorFile;
    Debug.UndoPrinting();
  } 
  if (keys[26] && keys[12]) {
    // Toggle keys
    isInversed = !isInversed;
    Debug.UndoPrinting();
  }
  if (keys[27] && keys[12]) {
    // Toggle keys
    isZeroNaN = !isZeroNaN;
    Debug.UndoPrinting();
  }
  if (keys[15] && keys[0]) {
    pulse_type = 0;
    Debug.UndoPrinting();
  }
  if (keys[15] && keys[1]) {
    pulse_type = 1;
    Debug.UndoPrinting();
  }
  if (keys[15] && keys[2]) {
    pulse_type = 2;
    Debug.UndoPrinting();
  }
  if (keys[15] && keys[3]) {
    pulse_type = 3;
    Debug.UndoPrinting();
  }
  if (keys[15] && keys[4]) {
    pulse_type = 4;
    Debug.UndoPrinting();
  }
  if (keys[28] && keys[11]) {
    MultiEndBuffer++;
    Debug.UndoPrinting();
  }
  if (keys[28] && keys[10]) {
    MultiEndBuffer--;
    Debug.UndoPrinting();
  }
  if (keys[24] && keys[0]) {
    reactivity_type = 0;
    Debug.UndoPrinting();
  }
  if (keys[24] && keys[1]) {
    reactivity_type = 1;
    Debug.UndoPrinting();
  }
  if (keys[24] && keys[2]) {
    reactivity_type = 2;
    Debug.UndoPrinting();
  }
  // Limit the number of FBM octaves to the int range [1-64].
  if (keys[29] && keys[11] && octaves < 64) {
    octaves++;
    Debug.UndoPrinting();
  }
  if (keys[29] && keys[10] && octaves > 1) {
    octaves--;
    Debug.UndoPrinting();
  }
  if (keys[14] && keys[12]) {
    isDitheringEnabled = !isDitheringEnabled;
    if (isDitheringEnabled) Debug.prStr("Dithering enabled");
    if (!isDitheringEnabled) Debug.prStr("Dithering disabled");
    opc.setDithering(isDitheringEnabled);
  }
  if (key == ' ' && isPlayer) { 
    sound[song].pause();
  }
  if (key == 'v' && isPlayer) {
    sound[song].play();
  }
  if (key == 'u' && isPlayer && !isWebPlayer && sound[song].position() <= sound[song].length()-MultiEndBuffer*AudioBufferSize && song < filename.length-1) {
    oldsong = song;
    song++;
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    metasound = sound[song].getMetaData();
    reinit_player_fft(sound[song].bufferSize(), sound[song].sampleRate());
    Debug.UndoPrinting();
  }
  if (key == 'y' && isPlayer && !isWebPlayer && sound[song].position() <= sound[song].length()-MultiEndBuffer*AudioBufferSize && song > 0) {
    oldsong = song;
    song--;
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    metasound = sound[song].getMetaData();
    reinit_player_fft(sound[song].bufferSize(), sound[song].sampleRate());
    Debug.UndoPrinting();
  }
  if (key == 'u' && isPlayer && isWebPlayer && song < filename.length-1) {
    oldsong = song;
    song++;
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    metasound = sound[song].getMetaData();
    reinit_player_fft(sound[song].bufferSize(), sound[song].sampleRate());
    Debug.UndoPrinting();
  }
  if (key == 'y' && isPlayer && isWebPlayer && song > 0) {
    oldsong = song;
    song--;
    sound[song] = minim.loadFile(filename[song], AudioBufferSize);
    metasound = sound[song].getMetaData();
    reinit_player_fft(sound[song].bufferSize(), sound[song].sampleRate());
    Debug.UndoPrinting();
  }
  if (key == 'k' && isPlayer && !isWebPlayer) sound[song].skip(100);
  if (key == 'j' && isPlayer && !isWebPlayer) sound[song].skip(-100);
}

void keyReleased() {
  if (key == '0')
    keys[0] = false;
  if (key == '1')
    keys[1] = false;
  if (key == '2')
    keys[2] = false;
  if (key == '3')
    keys[3] = false;
  if (key == '4')
    keys[4] = false;
  if (key == '5')
    keys[5] = false;
  if (key == '6')
    keys[6] = false;
  if (key == '7')
    keys[7] = false;
  if (key == '8')
    keys[8] = false;
  if (key == '9')
    keys[9] = false;
  if (key == '-')
    keys[10] = false;
  if (key == '+')
    keys[11] = false;
  if (key == 't')
    keys[12] = false;
  if (key == 'n')
    keys[13] = false;
  if (key == 'd')
    keys[14] = false;
  if (key == 'p')
    keys[15] = false;
  if (key == 'l')
    keys[16] = false;
  if (key == 'b')
    keys[17] = false;
  if (key == 'a')
    keys[18] = false;
  if (key == 's')
    keys[19] = false;
  if (key == 'q')
    keys[20] = false;
  if (key == 'h')
    keys[21] = false;
  if (key == 'e')
    keys[22] = false;
  if (key == 'w')
    keys[23] = false;
  if (key == 'r')
    keys[24] = false;
  if (key == 'c')
    keys[25] = false;
  if (key == 'i')
    keys[26] = false;
  if (key == 'z')
    keys[27] = false;
  if (key == 'm')
    keys[28] = false;
  if (key == 'o')
    keys[29] = false;
}

void mousePressed()
{
  // Choose a position to cue to based on where the user clicked.
  // the length() method returns the length of recording in milliseconds.
  if (isPlayer && !isWebPlayer && mouseY <=  height*0.03125) {
    float mousex = mouseX;
    int position = int(map(mousex, 0, width, 0, sound[song].length()));
    sound[song].cue(position);
  }
}