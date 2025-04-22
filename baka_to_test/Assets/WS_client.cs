using UnityEngine;
using WebSocketSharp;
using TMPro;
using UnityEngine.XR;
using System;

public class QuestWebSocketClient : MonoBehaviour
{
	private InputData _inputData;
    private WebSocket ws;
    public TextMeshProUGUI mytexto;
    public string serverUrl = "ws://192.168.0.175:81"; // Cambia esto a tu servidor WebSocket
    public double sensitivity = 0.95;

    void Start()
    {
        // Inicializa la conexión WebSocket
        _inputData = GetComponent<InputData>();
        ws = new WebSocket(serverUrl);
        ws.OnOpen += (sender, e) => Debug.Log("Conectado al servidor WebSocket.");
        ws.OnError += (sender, e) => Debug.LogError("Error: " + e.Message);
        ws.OnClose += (sender, e) => Debug.Log("Conexión cerrada.");

        ws.Connect();
        mytexto.SetText(serverUrl);
    }

    void Update()
    {
        if (ws.ReadyState == WebSocketState.Open)
        {
        	bool isPressed_A;
			if (_inputData._rightController.TryGetFeatureValue(CommonUsages.primaryButton, out isPressed_A) && isPressed_A)
				{
    // El botón primario fue presionado
	      			ws.Send("Puto");
	      			mytexto.SetText("A");

        		}
    		}
    		Vector2 joy_der;
    		if (_inputData._rightController.TryGetFeatureValue(CommonUsages.primary2DAxis, out joy_der))
    		{
    			if (Mathf.Abs(joy_der[1]) > sensitivity)
    			{
    				if(joy_der[1] > 0)
    				{
    					ws.Send("F");
    					mytexto.SetText("F");
    				}
    				else
    				{
    					ws.Send("B");
    					mytexto.SetText("B");
    				}
    			}
    		}
    		if (Mathf.Abs(joy_der[0]) > sensitivity)
    		{
    			if(joy_der[0] > 0)
    			{
   					ws.Send("R");
   					mytexto.SetText("R");
   				}
   				else
   				{
   					ws.Send("L");
   					mytexto.SetText("L");
   				}
   		}
    } 

    void OnApplicationQuit()
    {
        if (ws != null && ws.IsAlive)
        {
            ws.Close();
        }
    }

}