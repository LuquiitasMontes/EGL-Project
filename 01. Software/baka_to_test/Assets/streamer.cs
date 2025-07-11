using System;
using System.Collections;
using System.Net.Sockets;
using TMPro;
using UnityEngine;
using UnityEngine.Networking;

public class MJPEGStreamWithAutoIP : MonoBehaviour
{
    [Header("UI")]
    public TMP_Text statusText;

    [Header("Stream Settings")]
    public Renderer targetRenderer;
    public int streamPort = 8080;

    [Header("Conection Settings")]
    private int timeout_in = 2;

    private Texture2D videoTexture;
    private string cameraIP;
    private bool found = false;
    private bool isReconnecting = false;

    private int maxConcurrentRequests = 20;
    private int currentConcurrentRequests = 0;

    private void Start()
    {
        videoTexture = new Texture2D(2, 2);
        targetRenderer.material.mainTexture = videoTexture;

        StartCoroutine(DetectCameraIPConcurrent(ip =>
        {
            if (ip != null)
            {
                Debug.Log("Cámara encontrada en IP: " + ip);
                statusText.text = $"Cámara encontrada en: {ip}";
                cameraIP = ip;
                StartCoroutine(StartMJPEGStream());
            }
            else
            {
                Debug.LogWarning("No se encontró cámara en la red.");
                statusText.text = "No se encontró cámara en la red.";
            }
        }));
    }

    IEnumerator DetectCameraIPConcurrent(Action<string> onSuccess)
    {
        string localIP = GetLocalIPAddress();
        if (localIP == null)
        {
            Debug.LogError("No se pudo obtener IP local");
            onSuccess(null);
            yield break;
        }

        string subnet = localIP.Substring(0, localIP.LastIndexOf('.') + 1);
        found = false;

        for (int i = 2; i < 255; i++)
        {
            while (currentConcurrentRequests >= maxConcurrentRequests)
                yield return null;

            if (found)
                break;

            string testIP = subnet + i;
            StartCoroutine(TestIP(testIP, onSuccess));
        }

        while (!found)
            yield return null;
    }

    IEnumerator TestIP(string testIP, Action<string> onSuccess)
    {
        currentConcurrentRequests++;
        if (statusText != null)
            statusText.text = "Buscando en IP: " + testIP;

        string testUrl = $"http://{testIP}:{streamPort}/shot.jpg";

        using (UnityWebRequest request = UnityWebRequest.Head(testUrl))
        {
            request.timeout = timeout_in;

            yield return request.SendWebRequest();

            if (!found && request.result != UnityWebRequest.Result.ConnectionError &&
                request.result != UnityWebRequest.Result.ProtocolError)
            {
                found = true;
                onSuccess(testIP);
            }
        }

        currentConcurrentRequests--;
    }

    IEnumerator StartMJPEGStream()
    {
        if (string.IsNullOrEmpty(cameraIP))
            yield break;

        string streamUrl = $"http://{cameraIP}:{streamPort}/video";

        TcpClient client = new TcpClient();
        NetworkStream stream = null;
        System.IO.MemoryStream jpegData = new System.IO.MemoryStream();
        byte[] buffer = new byte[4096];
        bool connected = false;
        bool errorOccurred = false;
        string errorMsg = "";

        try
        {
            Uri uri = new Uri(streamUrl);
            client.Connect(uri.Host, uri.Port);
            stream = client.GetStream();

            string request = $"GET {uri.PathAndQuery} HTTP/1.1\r\nHost: {uri.Host}\r\nConnection: close\r\n\r\n";
            byte[] requestBytes = System.Text.Encoding.ASCII.GetBytes(request);
            stream.Write(requestBytes, 0, requestBytes.Length);

            connected = true;
        }
        catch (Exception e)
        {
            errorOccurred = true;
            errorMsg = e.Message;
        }

        while (connected && !errorOccurred && client.Connected)
        {
            int bytesRead = 0;
            try
            {
                bytesRead = stream.Read(buffer, 0, buffer.Length);
            }
            catch (Exception e)
            {
                errorOccurred = true;
                errorMsg = e.Message;
                break;
            }

            if (bytesRead <= 0)
                break;

            jpegData.Write(buffer, 0, bytesRead);

            byte[] dataArray = jpegData.ToArray();
            int startIndex = FindSequence(dataArray, new byte[] { 0xFF, 0xD8 });
            int endIndex = FindSequence(dataArray, new byte[] { 0xFF, 0xD9 });

            if (startIndex >= 0 && endIndex > startIndex)
            {
                int jpegLength = endIndex - startIndex + 2;
                byte[] jpegFrame = new byte[jpegLength];
                Array.Copy(dataArray, startIndex, jpegFrame, 0, jpegLength);

                // DESCARTA TODO LO ANTERIOR AL FRAME ACTUAL (modo agresivo)
                jpegData = new System.IO.MemoryStream();
                int leftoverStart = endIndex + 2;
                if (leftoverStart < dataArray.Length)
                    jpegData.Write(dataArray, leftoverStart, dataArray.Length - leftoverStart);

                DateTime frameTime = DateTime.Now;
                UnityMainThreadDispatcher.Instance().Enqueue(() =>
                {
                    if ((DateTime.Now - frameTime).TotalSeconds < 0.2)
                    {
                        videoTexture.LoadImage(jpegFrame);
                        videoTexture.Apply();
                    }
                });
            }
            else
            {
                if (jpegData.Length > 512000)
                {
                    jpegData = new System.IO.MemoryStream();
                    Debug.LogWarning("Buffer limpiado por exceso sin JPEG válido (modo agresivo)");
                }
            }

            yield return null;
        }

        client.Close();

        if (errorOccurred)
        {
            Debug.LogError("Error en stream MJPEG: " + errorMsg);
        }
        else
        {
            Debug.LogWarning("Conexión cerrada con la cámara MJPEG");
        }

        // Reintento automático
        if (!isReconnecting)
        {
            isReconnecting = true;
            StartCoroutine(ReconnectAfterDelay(3f)); // Esperar 3 segundos
        }
    }

    IEnumerator ReconnectAfterDelay(float seconds)
    {
        if (statusText != null)
            statusText.text = $"Reconectando en {seconds} segundos...";

        yield return new WaitForSeconds(seconds);

        if (statusText != null)
            statusText.text = "Reconectando...";

        if (!string.IsNullOrEmpty(cameraIP))
        {
            Debug.Log("Intentando reconectar al stream MJPEG...");
            StartCoroutine(StartMJPEGStream());
        }
        else
        {
            Debug.LogWarning("No se puede reconectar: IP de cámara no disponible.");
        }

        isReconnecting = false;
    }

    int FindSequence(byte[] array, byte[] sequence)
    {
        for (int i = 0; i < array.Length - sequence.Length; i++)
        {
            bool matched = true;
            for (int j = 0; j < sequence.Length; j++)
            {
                if (array[i + j] != sequence[j])
                {
                    matched = false;
                    break;
                }
            }
            if (matched) return i;
        }
        return -1;
    }

    string GetLocalIPAddress()
    {
        var host = System.Net.Dns.GetHostEntry(System.Net.Dns.GetHostName());
        foreach (var ip in host.AddressList)
        {
            if (ip.AddressFamily == AddressFamily.InterNetwork)
                return ip.ToString();
        }
        return null;
    }
}
