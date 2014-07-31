package trainer.modules.gui;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.SwingConstants;
import javax.swing.border.EmptyBorder;

import trainer.modules.decompressor.ShmStream;

public class TrainerGui extends JFrame {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	private JPanel contentPane;
	private ItemList<InboundListItem> inboundList = new ItemList<InboundListItem>();

	public void onInboundConnected(final ShmStream item) {
		inboundList.ensurePresent(new InboundListItem(item));
		item.openWindow();
	}

	public void onInboundDisconnected(final ShmStream item) {
		ScalaHelper.removeListItem(item, inboundList);
		item.closeWindow();
	}

	/**
	 * Create the frame.
	 */
	public TrainerGui() {
		setResizable(false);
		setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		setBounds(100, 100, 320, 300);
		contentPane = new JPanel();
		contentPane.setBorder(new EmptyBorder(5, 5, 5, 5));
		setContentPane(contentPane);
		contentPane.setLayout(null);

		JButton displayInboundBtn = new JButton("Display");
		displayInboundBtn.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				for (InboundListItem item : inboundList.selected()) {
					item.display();
				}
			}
		});
		displayInboundBtn.setToolTipText("Displays the selected stream");
		displayInboundBtn.setBounds(210, 96, 90, 28);
		contentPane.add(displayInboundBtn);

		inboundList.setBounds(15, 39, 180, 218);
		contentPane.add(inboundList);

		JLabel lblNewLabel = new JLabel("Streams");
		lblNewLabel.setHorizontalAlignment(SwingConstants.CENTER);
		lblNewLabel.setBounds(49, 11, 109, 14);
		contentPane.add(lblNewLabel);

		JButton closeStreamsBtn = new JButton("Close");
		closeStreamsBtn.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				for (InboundListItem item : inboundList.selected()) {
					item.close();
				}
			}
		});
		closeStreamsBtn.setToolTipText("Stops displaying the selected stream");
		closeStreamsBtn.setBounds(210, 135, 90, 28);
		contentPane.add(closeStreamsBtn);

	}

}
